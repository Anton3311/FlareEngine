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

#include "Flare/Scene/Prefab.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Transform.h"
#include "Flare/Scene/Hierarchy.h"

#include "FlareEditor/AssetManager/MeshImportSettings.h"
#include "FlareEditor/AssetManager/StaticMeshImporter.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include "FlareEditor/EditorLayer.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <unordered_map>

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

	static void ImportMaterials(const AssetMetadata& metadata,
		const aiScene* scene,
		const std::unordered_set<uint32_t>& usedMaterials,
		std::unordered_map<uint32_t, Ref<Material>>& outMaterials)
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
				else if (subAssetMetadata->Type == AssetType::MaterialsTable)
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

			outMaterials[i] = materialAsset;
		}
	}

	inline static glm::mat4 ConvertToColumnMajor(const aiMatrix4x4& matrix)
	{
		return glm::mat4(
			matrix.a1, matrix.b1, matrix.c1, matrix.d1,
			matrix.a2, matrix.b2, matrix.c2, matrix.d2,
			matrix.a3, matrix.b3, matrix.c3, matrix.d3,
			matrix.a4, matrix.b4, matrix.c4, matrix.d4);
	}

	class MeshHierarchyImporter
	{
	public:
		MeshHierarchyImporter(const aiScene& scene,
			const AssetMetadata& metadata,
			const SceneData& sceneData,
			const std::unordered_map<uint32_t, Ref<Material>>& materials)
			: m_Scene(scene), m_AssetMetadata(metadata), m_SceneData(sceneData), m_Materials(materials) {}

		void Import()
		{
			FLARE_PROFILE_FUNCTION();
			AssetHandle prefabHandle = NULL_ASSET_HANDLE;

			for (AssetHandle subAsset : m_AssetMetadata.SubAssets)
			{
				if (const auto* subAssetMetadata = AssetManager::GetAssetMetadata(subAsset))
				{
					if (subAssetMetadata->Type == AssetType::Prefab)
						prefabHandle = subAsset;
				}
			}

			Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());

			ECSContext& ecsContext = EditorLayer::GetInstance().GetECSContext();
			m_Prefab = CreateRef<Prefab>(ecsContext.Components, ecsContext.Archetypes);

			PrefabHierarchy& prefabHierarchy = m_Prefab->GetHierarchy();

			{
				std::vector<ComponentId> defaultArchetypeComponents =
				{
					COMPONENT_ID(TransformComponent),
					COMPONENT_ID(LocalTransform),
					COMPONENT_ID(Parent),
					COMPONENT_ID(Children),
					COMPONENT_ID(MeshRenderer)
				};

				m_DefaultNodeArchetype = prefabHierarchy
					.GetCompatibleArchetypes()
					.FindOrCreateArchetype(Span<ComponentId>::FromVector(defaultArchetypeComponents))->Id;
			}

			{
				std::vector<ComponentId> defaultArchetypeComponents =
				{
					COMPONENT_ID(TransformComponent),
					COMPONENT_ID(LocalTransform),
					COMPONENT_ID(Children),
					COMPONENT_ID(MeshRenderer)
				};

				m_DefaultRootArchetype = prefabHierarchy
					.GetCompatibleArchetypes()
					.FindOrCreateArchetype(Span<ComponentId>::FromVector(defaultArchetypeComponents))->Id;
			}

			VisitNode(*m_Scene.mRootNode, PrefabHierarchy::Node::INVALID_PARENT_NODE);

			prefabHierarchy.EnsureAllocated();
			prefabHierarchy.InitializeEntities();

			{
				const Math::AffineTransform identityTransform;

				CopyTransforms(*m_Scene.mRootNode, identityTransform);
			}

			if (assetManager->IsAssetHandleValid(prefabHandle))
				assetManager->SetLoadedAsset(prefabHandle, m_Prefab);
			else
				assetManager->ImportMemoryOnlyAsset("Prefab", m_Prefab, m_AssetMetadata.Handle);
		}
	private:
		void VisitNode(const aiNode& node, size_t parentIndex)
		{
			FLARE_PROFILE_FUNCTION();
			auto& hierarchy = m_Prefab->GetHierarchy();

			size_t nodeIndex = hierarchy.GetNodes().size();
			nodeIndexMap[&node] = nodeIndex;

			if (parentIndex == PrefabHierarchy::Node::INVALID_PARENT_NODE)
				hierarchy.AddEntity(m_DefaultRootArchetype, parentIndex);
			else
				hierarchy.AddEntity(m_DefaultNodeArchetype, parentIndex);

			for (uint32_t child = 0; child < node.mNumChildren; child++)
			{
				VisitNode(*node.mChildren[child], nodeIndex);
			}
		}

		void CopyTransforms(const aiNode& node, const Math::AffineTransform& parentTransform)
		{
			FLARE_PROFILE_FUNCTION();
			PrefabHierarchy& hierarchy = m_Prefab->GetHierarchy(); 
			size_t nodeIndex = nodeIndexMap[&node];

			Math::AffineTransform* globalTransform = hierarchy.TryGetNodeComponent<TransformComponent>(nodeIndex);
			Math::AffineTransform* localTransform = hierarchy.TryGetNodeComponent<LocalTransform>(nodeIndex);

			glm::mat4 transformRelativeToParent = ConvertToColumnMajor(node.mTransformation);
			Math::DecomposeTransform(transformRelativeToParent, localTransform->Position, localTransform->Rotation, localTransform->Scale);

			*globalTransform = *localTransform;
			globalTransform->ApplyTransform(parentTransform);

			if (node.mNumMeshes > 0)
			{
				MeshRenderer* meshRenderer = hierarchy.TryGetNodeComponent<MeshRenderer>(nodeIndex);
				FLARE_CORE_ASSERT(meshRenderer);

				const NodeMesh& meshData = m_SceneData.NodeToMesh.at(&node);
				meshRenderer->Mesh = meshData.Mesh;

				for (uint32_t index : meshData.MaterialIndices)
				{
					meshRenderer->Materials.push_back(m_Materials.at(index));
				}
			}

			for (uint32_t child = 0; child < node.mNumChildren; child++)
			{
				const aiNode* childNode = node.mChildren[child];
				CopyTransforms(*childNode, *globalTransform);
			}
		}
	private:
		const aiScene& m_Scene;
		const SceneData& m_SceneData;
		const std::unordered_map<uint32_t, Ref<Material>>& m_Materials;

		std::unordered_map<const aiNode*, size_t> nodeIndexMap;

		ArchetypeId m_DefaultNodeArchetype = INVALID_ARCHETYPE_ID;
		ArchetypeId m_DefaultRootArchetype = INVALID_ARCHETYPE_ID;

		Ref<Prefab> m_Prefab = nullptr;
		const AssetMetadata& m_AssetMetadata;
	};

	Ref<Mesh> MeshImporter::ImportMesh(const AssetMetadata& metadata)
	{
		FLARE_PROFILE_FUNCTION();
		std::underlying_type_t<aiPostProcessSteps> postProcessSteps = aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipWindingOrder;
		if (metadata.Path.extension() == ".fbx")
			postProcessSteps |= aiProcess_FlipUVs;

		MeshImportSettings importSettings{};
		MeshImportSettingsSerializer::Deserialize(metadata.Handle, importSettings);

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

		StaticMeshImporter staticMeshImporter(scene, importSettings);
		staticMeshImporter.Import();

		const SceneData& data = staticMeshImporter.GetSceneData();

		std::unordered_map<uint32_t, Ref<Material>> importedMaterials;

		if (importSettings.ImportMaterials)
		{
			ImportMaterials(metadata, scene, data.UsedMaterials, importedMaterials);
		}

		{
			MeshHierarchyImporter hierarchyImporter(*scene, metadata, staticMeshImporter.GetSceneData(), importedMaterials);
			hierarchyImporter.Import();
		}

#if 0


		MemorySpan indices = MemorySpan();
		if (data.IndexFormat == IndexBuffer::IndexFormat::UInt16)
		{
			indices = MemorySpan::FromVector(data.Indices16);
		}
		else
		{
			indices = MemorySpan::FromVector(data.Indices32);
		}

		Ref<Mesh> mesh = CreateRef<Mesh>(indices,
			data.IndexFormat,
			Span(data.Vertices.data(), data.Vertices.size()),
			Span(data.Normals.data(), data.Normals.size()),
			Span(data.Tangents.data(), data.Tangents.size()),
			Span(data.UVs.data(), data.UVs.size()));

		mesh->SetDebugName(metadata.Name);

		for (const auto& subMesh : data.SubMeshes)
		{
			mesh->AddSubMesh(subMesh);
		}
#endif

		return nullptr;
	}
}
