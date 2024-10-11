#include "PCH.h"

#include "MeshHierarchyImporter.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Scene/Prefab.h"
#include "Flare/Scene/Transform.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Hierarchy.h"

#include "FlareECS/ECSContext.h"
#include "FlareECS/Entity/Component.h"

#include "FlareEditor/AssetManager/StaticMeshImporter.h"
#include "FlareEditor/EditorLayer.h"

#include <assimp/scene.h>

namespace Flare
{
	MeshHierarchyImporter::MeshHierarchyImporter(const aiScene& scene,
		const SceneData& sceneData,
		const AssetMetadata& assetMetadata,
		const std::unordered_map<uint32_t,
		Ref<Material>>&materials,
		MeshImportSettings& importSettings)
		: m_Scene(scene), m_AssetMetadata(assetMetadata), m_SceneData(sceneData), m_Materials(materials), m_ImportSettings(importSettings)
	{
	}

	void MeshHierarchyImporter::Import()
	{
		FLARE_PROFILE_FUNCTION();
		ECSContext& ecsContext = EditorLayer::GetInstance().GetECSContext();
		m_Prefab = Ref<Prefab>::New(ecsContext.Components, ecsContext.Archetypes, PrefabFlags::Generated, m_AssetMetadata.Handle);

		PrefabHierarchy& prefabHierarchy = m_Prefab->GetHierarchy();

		for (uint32_t i = 0; i < m_Scene.mNumLights; i++)
		{
			m_LightNameToIndex.emplace(std::string_view(m_Scene.mLights[i]->mName.C_Str()), i);
		}

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

		{
			std::vector<ComponentId> defaultArchetypeComponents =
			{
				COMPONENT_ID(TransformComponent),
				COMPONENT_ID(LocalTransform),
				COMPONENT_ID(Children),
				COMPONENT_ID(Parent),
				COMPONENT_ID(PointLight)
			};

			m_PointLightArchetype = prefabHierarchy
				.GetCompatibleArchetypes()
				.FindOrCreateArchetype(Span<ComponentId>::FromVector(defaultArchetypeComponents))->Id;
		}

		{
			std::vector<ComponentId> defaultArchetypeComponents =
			{
				COMPONENT_ID(TransformComponent),
				COMPONENT_ID(LocalTransform),
				COMPONENT_ID(Children),
				COMPONENT_ID(Parent),
				COMPONENT_ID(SpotLight)
			};

			m_SpotLightArchetype = prefabHierarchy
				.GetCompatibleArchetypes()
				.FindOrCreateArchetype(Span<ComponentId>::FromVector(defaultArchetypeComponents))->Id;
		}

		VisitNode(*m_Scene.mRootNode, PrefabHierarchy::Node::INVALID_PARENT_NODE);

		prefabHierarchy.EnsureAllocated();
		prefabHierarchy.InitializeEntities();

		{
			const Math::AffineTransform identityTransform;

			CopyTransform(*m_Scene.mRootNode, identityTransform);

			CopyComponentData(*m_Scene.mRootNode);
		}
	}

	MeshHierarchyImporter::NodeType MeshHierarchyImporter::GetNodeType(const aiNode& node) const
	{
		if (node.mNumMeshes > 0)
			return NodeType::Mesh;

		auto it = m_LightNameToIndex.find(node.mName.C_Str());
		if (it != m_LightNameToIndex.end())
		{
			switch (m_Scene.mLights[it->second]->mType)
			{
			case aiLightSource_POINT:
				return NodeType::PointLight;
			case aiLightSource_SPOT:
				return NodeType::SpotLight;
			default:
				FLARE_CORE_ERROR("Unsupported light source type");
				return NodeType::Empty;
			}
		}

		return NodeType::Empty;
	}

	void MeshHierarchyImporter::VisitNode(const aiNode& node, size_t parentIndex)
	{
		FLARE_PROFILE_FUNCTION();
		auto& hierarchy = m_Prefab->GetHierarchy();

		size_t nodeIndex = hierarchy.GetNodes().size();
		m_NodeIndexMap[&node] = nodeIndex;

		NodeType nodeType = GetNodeType(node);

		if (parentIndex == PrefabHierarchy::Node::INVALID_PARENT_NODE)
			hierarchy.AddEntity(m_DefaultRootArchetype, parentIndex);
		else if (nodeType == NodeType::PointLight)
			hierarchy.AddEntity(m_PointLightArchetype, parentIndex);
		else if (nodeType == NodeType::SpotLight)
			hierarchy.AddEntity(m_SpotLightArchetype, parentIndex);
		else
			hierarchy.AddEntity(m_DefaultNodeArchetype, parentIndex);

		for (uint32_t child = 0; child < node.mNumChildren; child++)
		{
			VisitNode(*node.mChildren[child], nodeIndex);
		}
	}

	void MeshHierarchyImporter::CopyComponentData(const aiNode& node)
	{
		FLARE_PROFILE_FUNCTION();

		CopyMeshComponent(node);
		CopyLightData(node);

		for (uint32_t child = 0; child < node.mNumChildren; child++)
		{
			CopyComponentData(*node.mChildren[child]);
		}
	}

	void MeshHierarchyImporter::CopyTransform(const aiNode& node, const Math::AffineTransform& parentTransform)
	{
		FLARE_PROFILE_FUNCTION();
		PrefabHierarchy& hierarchy = m_Prefab->GetHierarchy();
		size_t nodeIndex = m_NodeIndexMap[&node];

		Math::AffineTransform* globalTransform = hierarchy.TryGetNodeComponent<TransformComponent>(nodeIndex);
		Math::AffineTransform* localTransform = hierarchy.TryGetNodeComponent<LocalTransform>(nodeIndex);

		glm::mat4 transformRelativeToParent = ConvertToColumnMajor(node.mTransformation);
		Math::DecomposeTransform(transformRelativeToParent, localTransform->Position, localTransform->Rotation, localTransform->Scale);

		*globalTransform = *localTransform;
		globalTransform->ApplyTransform(parentTransform);
		for (uint32_t child = 0; child < node.mNumChildren; child++)
		{
			const aiNode* childNode = node.mChildren[child];
			CopyTransform(*childNode, *globalTransform);
		}
	}

	void MeshHierarchyImporter::CopyMeshComponent(const aiNode& node)
	{
		FLARE_PROFILE_FUNCTION();

		PrefabHierarchy& hierarchy = m_Prefab->GetHierarchy();
		size_t nodeIndex = m_NodeIndexMap[&node];

		if (node.mNumMeshes > 0)
		{
			MeshRenderer* meshRenderer = hierarchy.TryGetNodeComponent<MeshRenderer>(nodeIndex);
			FLARE_CORE_ASSERT(meshRenderer);

			const NodeMesh& meshData = m_SceneData.NodeToMesh.at(&node);
			meshRenderer->Mesh = meshData.Mesh;

			for (uint32_t index : meshData.MaterialIndices)
			{
				auto it = m_Materials.find(index);
				if (it != m_Materials.end())
				{
					meshRenderer->Materials.push_back(it->second);
				}
			}
		}
	}

	void MeshHierarchyImporter::CopyLightData(const aiNode& node)
	{
		FLARE_PROFILE_FUNCTION();

		PrefabHierarchy& hierarchy = m_Prefab->GetHierarchy();
		size_t nodeIndex = m_NodeIndexMap[&node];

		auto it = m_LightNameToIndex.find(node.mName.C_Str());
		if (it == m_LightNameToIndex.end())
			return;

		const aiLight& light = *m_Scene.mLights[it->second];
		switch (light.mType)
		{
		case aiLightSource_POINT:
		{
			PointLight* pointLight = hierarchy.TryGetNodeComponent<PointLight>(nodeIndex);
			FLARE_CORE_ASSERT(pointLight);

			pointLight->Color = glm::vec3(light.mColorDiffuse.r, light.mColorDiffuse.g, light.mColorDiffuse.b);
			pointLight->Intensity = glm::length(pointLight->Color);
			pointLight->Color /= pointLight->Intensity;
			break;
		}
		case aiLightSource_SPOT:
		{
			SpotLight* spotLight = hierarchy.TryGetNodeComponent<SpotLight>(nodeIndex);
			FLARE_CORE_ASSERT(spotLight);

			spotLight->InnerAngle = glm::degrees(light.mAngleInnerCone);
			spotLight->OuterAngle = glm::degrees(light.mAngleOuterCone);

			spotLight->Color = glm::vec3(light.mColorDiffuse.r, light.mColorDiffuse.g, light.mColorDiffuse.b);
			spotLight->Intensity = glm::length(spotLight->Color);
			spotLight->Color /= spotLight->Intensity;
			break;
		}
		default:
			FLARE_CORE_ERROR("Unsupported light type");
			break;
		}
	}
}
