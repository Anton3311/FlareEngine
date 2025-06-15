#include "PCH.h"

#include "MeshImporter.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Mesh.h"
#include "Flare/Renderer/MeshSource.h"
#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/Scene/Prefab.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Transform.h"
#include "Flare/Scene/Hierarchy.h"

#include "Flare/Serialization/Serialization.h"

#include "FlareEditor/AssetManager/PrefabImporter.h"
#include "FlareEditor/AssetManager/MeshImportSettings.h"
#include "FlareEditor/AssetManager/MeshHierarchyImporter.h"
#include "FlareEditor/AssetManager/StaticMeshImporter.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include "FlareEditor/EditorLayer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <unordered_map>
#include <fstream>

#include <yaml-cpp/yaml.h>

namespace Flare
{
	static AssetHandle FindTextureByPath(std::string_view path, const AssetMetadata& metadata, const Ref<EditorAssetManager>& assetManager)
	{
		FLARE_PROFILE_FUNCTION();
		AssetHandle textureHandle = NULL_ASSET_HANDLE;
		if (path.size() > 0)
		{
			std::filesystem::path texturePath = metadata.Path.parent_path() / path;
			if (std::filesystem::exists(texturePath))
			{
				std::optional<AssetHandle> handle = assetManager->FindAssetByPath(texturePath);
				if (handle)
					textureHandle = handle.value();
				else
					textureHandle = assetManager->ImportAsset(texturePath);
			}
		}

		return textureHandle;
	}

	static void TrySetMaterialTexture(std::optional<uint32_t> propertyIndex, const Ref<Material>& material, AssetHandle handle)
	{
		FLARE_PROFILE_FUNCTION();
		if (propertyIndex)
		{
			if (handle != NULL_ASSET_HANDLE)
				material->SetTextureProperty(*propertyIndex, AssetManager::GetAsset<Texture>(handle));
		}
	}

	static void ImportMaterials(const AssetMetadata& metadata,
		const aiScene* scene,
		const std::unordered_set<uint32_t>& usedMaterials,
		std::unordered_map<uint32_t, Ref<Material>>& outMaterials)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<EditorAssetManager> assetManager = AssetManager::GetInstance().As<EditorAssetManager>();
		std::optional<AssetHandle> defaultShader = ShaderLibrary::FindShader("Surface");
		std::optional<AssetHandle> specularSurfaceShader = ShaderLibrary::FindShader("SpecularSurface");

		std::unordered_map<std::string, AssetHandle> nameToHandle;

		for (AssetHandle subAsset : metadata.SubAssets)
		{
			if (const auto* subAssetMetadata = AssetManager::GetAssetMetadata(subAsset))
			{
				if (subAssetMetadata->Type == AssetType::Material)
					nameToHandle[subAssetMetadata->Name] = subAsset;
			}
		}

		if (!defaultShader)
		{
			FLARE_CORE_ERROR("Failed to find 'Mesh' shader");
			return;
		}

		auto getMaterialTexture = [&](const aiMaterial& material, aiTextureType type) -> AssetHandle
		{
			FLARE_PROFILE_FUNCTION();

			aiTextureMapping mapping;
			uint32_t uvIndex;
			aiString path;

			aiReturn result = material.GetTexture(type, 0, &path, &mapping, &uvIndex);
			if (result == aiReturn_SUCCESS)
				return FindTextureByPath(std::string_view(path.C_Str(), path.length), metadata, assetManager);

			return NULL_ASSET_HANDLE;
		};

		for (uint32_t i : usedMaterials)
		{
			FLARE_PROFILE_SCOPE("ImportSingleMaterial");
			auto& material = scene->mMaterials[i];

			std::string name = material->GetName().C_Str();
			if (name.empty())
				name = fmt::format("Material {}", i);

			AssetHandle baseColorTextureHandle = getMaterialTexture(*material, aiTextureType_BASE_COLOR);
			AssetHandle normalMapHandle = getMaterialTexture(*material, aiTextureType_NORMALS);
			AssetHandle roughnessMapHandle = getMaterialTexture(*material, aiTextureType_DIFFUSE_ROUGHNESS);
			AssetHandle metallicMapHandle = getMaterialTexture(*material, aiTextureType_METALNESS);
			AssetHandle specularMapHandle = getMaterialTexture(*material, aiTextureType_SPECULAR);
			AssetHandle emissionMapHandle = getMaterialTexture(*material, aiTextureType_EMISSIVE);

			if (baseColorTextureHandle == NULL_ASSET_HANDLE)
			{
				baseColorTextureHandle = getMaterialTexture(*material, aiTextureType_DIFFUSE);
			}

			std::optional<AssetHandle> selectedSurfaceShader;
			if (specularMapHandle == NULL_ASSET_HANDLE)
				selectedSurfaceShader = defaultShader;
			else
				selectedSurfaceShader = specularSurfaceShader;

			if (!selectedSurfaceShader.has_value())
			{
				FLARE_CORE_ERROR("Failed to select surface shader for material {}", material->GetName().C_Str());
				outMaterials[i] = Renderer::GetErrorMaterial();
				continue;
			}

			aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
			float roughness = 1.0f;
			float metallic = 0.0f;
			aiColor3D emission(0.0f);

			material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);
			material->Get(AI_MATKEY_METALLIC_FACTOR, metallic);
			material->Get(AI_MATKEY_COLOR_EMISSIVE, emission);

			Ref<Material> materialAsset = Material::Create(*selectedSurfaceShader);

			std::optional<uint32_t> colorProperty;
			std::optional<uint32_t> emissionProperty;
			std::optional<uint32_t> roughnessProperty;
			std::optional<uint32_t> textureProperty;
			std::optional<uint32_t> normalMapProperty;
			std::optional<uint32_t> roughnessMapProperty;
			std::optional<uint32_t> metallicProperty;
			std::optional<uint32_t> metallicMapProperty;
			std::optional<uint32_t> specularMapProperty;
			std::optional<uint32_t> emissionMapProperty;

			Ref<Shader> shader = materialAsset->GetShader();
			if (shader != nullptr && shader->IsLoaded())
			{
				colorProperty = shader->GetPropertyIndex("u_Material.Color");
				emissionProperty = shader->GetPropertyIndex("u_Material.Emission");
				roughnessProperty = shader->GetPropertyIndex("u_Material.Roughness");
				textureProperty = shader->GetPropertyIndex("u_Texture");
				normalMapProperty = shader->GetPropertyIndex("u_NormalMap");
				roughnessMapProperty = shader->GetPropertyIndex("u_RoughnessMap");
				metallicProperty = shader->GetPropertyIndex("u_Material.Metallic");
				metallicMapProperty = shader->GetPropertyIndex("u_MetallicMap");
				specularMapProperty = shader->GetPropertyIndex("u_SpecularMap");
				emissionMapProperty = shader->GetPropertyIndex("u_EmissionMap");
			}

			if (colorProperty)
				materialAsset->WritePropertyValue(*colorProperty, glm::vec4(color.r, color.g, color.b, color.a));
			if (roughnessProperty)
				materialAsset->WritePropertyValue(*roughnessProperty, roughness);
			if (metallicProperty)
				materialAsset->WritePropertyValue(*metallicProperty, metallic);
			if (emissionProperty)
			{
				constexpr float EMISSION_INTENSITY = 100.0f;
				glm::vec3 emissionValue = glm::vec3(emission.r, emission.g, emission.b) * EMISSION_INTENSITY;
				materialAsset->WritePropertyValue<glm::vec3>(*emissionProperty, emissionValue);
			}

			TrySetMaterialTexture(textureProperty, materialAsset, baseColorTextureHandle);
			TrySetMaterialTexture(normalMapProperty, materialAsset, normalMapHandle);
			TrySetMaterialTexture(roughnessMapProperty, materialAsset, roughnessMapHandle);
			TrySetMaterialTexture(metallicMapProperty, materialAsset, metallicMapHandle);
			TrySetMaterialTexture(specularMapProperty, materialAsset, specularMapHandle);
			TrySetMaterialTexture(emissionMapProperty, materialAsset, emissionMapHandle);

			auto it = nameToHandle.find(name);
			if (it != nameToHandle.end())
			{
				assetManager->SetLoadedAsset(it->second, materialAsset);
			}
			else
			{
				assetManager->ImportMemoryOnlyAsset(name, materialAsset, metadata.Handle);
			}

			outMaterials[i] = materialAsset;
		}
	}

	static const aiScene* ImportScene(Assimp::Importer& importer, const std::filesystem::path& path)
	{
		std::underlying_type_t<aiPostProcessSteps> postProcessSteps = aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipWindingOrder;
		if (path.extension() == ".fbx")
			postProcessSteps |= aiProcess_FlipUVs;

		const aiScene* scene = nullptr;

		{
			FLARE_PROFILE_SCOPE("ReadFile");
			scene = importer.ReadFile(path.string(), postProcessSteps);
		}

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			FLARE_CORE_ERROR("Failed to load mesh {}: {}", path.generic_string(), importer.GetErrorString());
			return nullptr;
		}

		return scene;
	}

	Ref<Mesh> MeshImporter::ImportMesh(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();

		AssetHandle meshSource = NULL_ASSET_HANDLE;
		if (!DeserializeMeshSource(metadata.Path, meshSource))
			return nullptr;

		if (!AssetManager::IsAssetHandleValid(meshSource))
		{
			FLARE_CORE_ERROR("Failed to import: Mesh source handle is invalid");
			return nullptr;
		}

		const AssetMetadata& sourceMetadata = *AssetManager::GetAssetMetadata(meshSource);

		MeshImportSettings importSettings{};
		importSettings.PreserveHierarchy = false;

		Assimp::Importer importer;
		const aiScene* scene = ImportScene(importer, sourceMetadata.Path);

		if (!scene)
		{
			FLARE_CORE_ERROR("Failed to load mesh {}: {}", sourceMetadata.Path.generic_string(), importer.GetErrorString());
			return nullptr;
		}

		StaticMeshImporter staticMeshImporter(scene, importSettings);
		staticMeshImporter.Import();

		const SceneData& data = staticMeshImporter.GetSceneData();

		std::unordered_map<uint32_t, Ref<Material>> importedMaterials;

		Ref<Mesh> mesh = nullptr;
		
		if (data.IndexFormat == IndexFormat::UInt16)
		{
		 	mesh = Ref<Mesh>::New(MemorySpan(data.Indices16, data.IndexCount),
				data.IndexFormat,
				Span<const glm::vec3>(data.Vertices, data.VertexCount),
				Span<const glm::vec3>(data.Normals, data.VertexCount),
				Span<const glm::vec3>(data.Tangents, data.VertexCount),
				Span<const glm::vec2>(data.UVs, data.VertexCount),
				Span<const SubMesh>(data.SubMeshes, data.SubMeshCount));
		}
		else
		{
			MemorySpan indices = MemorySpan(data.Indices32, data.IndexCount);
			mesh = Ref<Mesh>::New(indices,
				data.IndexFormat,
				Span<const glm::vec3>(data.Vertices, data.VertexCount),
				Span<const glm::vec3>(data.Normals, data.VertexCount),
				Span<const glm::vec3>(data.Tangents, data.VertexCount),
				Span<const glm::vec2>(data.UVs, data.VertexCount));
		}

		mesh->SetDebugName(metadata.Name);

		return mesh;
	}

	Ref<Prefab> MeshImporter::ImportAsPrefab(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();

		Assimp::Importer importer;
		const aiScene* scene = ImportScene(importer, metadata.Path);

		if (!scene)
		{
			FLARE_CORE_ERROR("Failed to import mesh");
			return nullptr;
		}

		MeshImportSettings importSettings{};
		importSettings.ImportMaterials = true;

		StaticMeshImporter staticMeshImporter(scene, importSettings);
		staticMeshImporter.Import();

		std::unordered_map<uint32_t, Ref<Material>> importedMaterials;

		if (importSettings.ImportMaterials)
		{
			ImportMaterials(metadata, scene, staticMeshImporter.GetSceneData().UsedMaterials, importedMaterials);
		}

		MeshHierarchyImporter hierarchyImporter(*scene,
			staticMeshImporter.GetSceneData(),
			metadata,
			importedMaterials,
			importSettings);

		hierarchyImporter.Import();

		return hierarchyImporter.GetImportedPrefab();
	}

	Ref<MeshSource> MeshImporter::ImportMeshSource(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();
		return Ref<MeshSource>::New();
	}

	void MeshImporter::SerializeMesh(AssetHandle meshHandle, AssetHandle meshSourceHandle)
	{
		SerializeMesh(AssetManager::GetAssetMetadata(meshHandle)->Path, meshSourceHandle);
	}

	void MeshImporter::SerializeMesh(const std::filesystem::path& path, AssetHandle meshSourceHandle)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(meshSourceHandle));

		YAML::Emitter emitter;

		emitter << YAML::BeginMap;
		emitter << YAML::Key << "Source" << YAML::Value << meshSourceHandle;
		emitter << YAML::EndMap;

		std::ofstream output(path);
		output << emitter.c_str();
	}

	bool MeshImporter::DeserializeMeshSource(const std::filesystem::path& path, AssetHandle& outMeshSourceHandle)
	{
		FLARE_PROFILE_FUNCTION();

		if (!std::filesystem::exists(path))
			return false;

		try
		{
			YAML::Node root = YAML::LoadFile(path.string());

			if (YAML::Node sourceNode = root["Source"])
			{
				outMeshSourceHandle = sourceNode.as<AssetHandle>(outMeshSourceHandle);
			}
		}
		catch (std::exception& exception)
		{
			FLARE_CORE_ERROR("Failed to deserialize mesh source {}: {}", path.string(), exception.what());
			return false;
		}

		return true;
	}
}
