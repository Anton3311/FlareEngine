#include "MeshImporter.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/MaterialsTable.h"
#include "Flare/Renderer/Mesh.h"
#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Renderer.h"

#include "FlareEditor/AssetManager/StaticMeshImporter.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

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

	static void TrySetMaterialTexture(std::optional<uint32_t> propertyIndex, const Ref<Material>& material, AssetHandle handle, const Ref<Texture>& defaultValue)
	{
		FLARE_PROFILE_FUNCTION();
		if (propertyIndex)
		{
			if (handle == NULL_ASSET_HANDLE)
				material->SetTextureProperty(*propertyIndex, defaultValue);
			else
				material->SetTextureProperty(*propertyIndex, AssetManager::GetAsset<Texture>(handle));
		}
	}

	static void ImportMaterials(const AssetMetadata& metadata, const aiScene* scene, const std::vector<uint32_t>& usedMaterials)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
		std::optional<AssetHandle> defaultShader = ShaderLibrary::FindShader("Mesh");

		std::unordered_map<std::string, AssetHandle> nameToHandle;
		AssetHandle materialsTableHandle;

		for (AssetHandle subAsset : metadata.SubAssets)
		{
			if (const auto* subAssetMetadata = AssetManager::GetAssetMetadata(subAsset))
			{
				if (subAssetMetadata->Type == AssetType::Material)
					nameToHandle[subAssetMetadata->Name] = subAsset;
				else
					materialsTableHandle = subAsset;
			}
		}

		if (!defaultShader)
		{
			FLARE_CORE_ERROR("Failed to find 'Mesh' shader");
			return;
		}

		std::optional<uint32_t> colorProperty;
		std::optional<uint32_t> roughnessProperty;
		std::optional<uint32_t> textureProperty;
		std::optional<uint32_t> normalMapProperty;
		std::optional<uint32_t> roughnessMapProperty;

		Ref<Shader> shader = AssetManager::GetAsset<Shader>(defaultShader.value());
		if (shader != nullptr && shader->IsLoaded())
		{
			colorProperty = shader->GetPropertyIndex("u_Material.Color");
			roughnessProperty = shader->GetPropertyIndex("u_Material.Roughness");
			textureProperty = shader->GetPropertyIndex("u_Texture");
			normalMapProperty = shader->GetPropertyIndex("u_NormalMap");
			roughnessMapProperty = shader->GetPropertyIndex("u_RoughnessMap");
		}

		Ref<MaterialsTable> materialsTable = CreateRef<MaterialsTable>();
		if (AssetManager::IsAssetHandleValid(materialsTableHandle))
			assetManager->SetLoadedAsset(materialsTableHandle, materialsTable);
		else
			assetManager->ImportMemoryOnlyAsset("DefaultMaterialsTable", materialsTable, metadata.Handle);

		materialsTable->Materials.reserve(usedMaterials.size());

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

			AssetHandle baseColorTextureHandle = NULL_ASSET_HANDLE;
			AssetHandle normalMapHandle = NULL_ASSET_HANDLE;
			AssetHandle roughnessMapHandle = NULL_ASSET_HANDLE;

			baseColorTextureHandle = getMaterialTexture(*material, aiTextureType_BASE_COLOR);
			normalMapHandle = getMaterialTexture(*material, aiTextureType_NORMALS);
			roughnessMapHandle = getMaterialTexture(*material, aiTextureType_DIFFUSE_ROUGHNESS);

			if (baseColorTextureHandle == NULL_ASSET_HANDLE)
			{
				baseColorTextureHandle = getMaterialTexture(*material, aiTextureType_DIFFUSE);
			}

			aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
			material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
			float roughness = 1.0f;
			material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

			Ref<Material> materialAsset = Material::Create(defaultShader.value());

			if (colorProperty)
				materialAsset->WritePropertyValue(*colorProperty, glm::vec4(color.r, color.g, color.b, color.a));
			if (roughnessProperty)
				materialAsset->WritePropertyValue(*roughnessProperty, roughness);

			TrySetMaterialTexture(textureProperty, materialAsset, baseColorTextureHandle, Renderer::GetWhiteTexture());
			TrySetMaterialTexture(normalMapProperty, materialAsset, normalMapHandle, Renderer::GetDefaultNormalMap());
			TrySetMaterialTexture(roughnessMapProperty, materialAsset, roughnessMapHandle, Renderer::GetWhiteTexture());

			auto it = nameToHandle.find(name);
			if (it != nameToHandle.end())
			{
				assetManager->SetLoadedAsset(it->second, materialAsset);
				materialsTable->Materials.push_back(it->second);
			}
			else
			{
				AssetHandle handle = assetManager->ImportMemoryOnlyAsset(name, materialAsset, metadata.Handle);
				materialsTable->Materials.push_back(handle);
			}

		}
	}

	Ref<Mesh> MeshImporter::ImportMesh(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();
		std::underlying_type_t<aiPostProcessSteps> postProcessSteps = aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipWindingOrder;
		if (metadata.Path.extension() == ".fbx")
			postProcessSteps |= aiProcess_FlipUVs;

		Assimp::Importer importer;
		const aiScene* scene = nullptr;

		{
			FLARE_PROFILE_SCOPE("ReadFile");
			scene = importer.ReadFile(metadata.Path.string(), postProcessSteps);
		}

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			FLARE_CORE_ERROR("Failed to load mesh {}: {}", metadata.Path.generic_string(), importer.GetErrorString());
			return nullptr;
		}

		StaticMeshImporter staticMeshImporter(scene);
		staticMeshImporter.Import();

		const SceneData& data = staticMeshImporter.GetSceneData();

		MemorySpan indices = MemorySpan();
		if (data.IndexFormat == IndexBuffer::IndexFormat::UInt16)
		{
			indices = MemorySpan::FromVector(data.Indices16);
		}
		else
		{
			indices = MemorySpan::FromVector(data.Indices32);
		}

		Ref<Mesh> mesh = Mesh::Create(indices, data.IndexFormat,
			Span(data.Vertices.data(), data.Vertices.size()),
			Span(data.Normals.data(), data.Normals.size()),
			Span(data.Tangents.data(), data.Tangents.size()),
			Span(data.UVs.data(), data.UVs.size()));

		mesh->SetDebugName(metadata.Name);

		for (const auto& subMesh : data.SubMeshes)
		{
			mesh->AddSubMesh(subMesh);
		}

		ImportMaterials(metadata, scene, data.UsedMaterials);

		return mesh;
	}
}
