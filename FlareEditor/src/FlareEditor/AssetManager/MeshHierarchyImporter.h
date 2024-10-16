#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Entity/Archetypes.h"

#include <unordered_map>
#include <string_view>

struct aiScene;
struct aiNode;
struct aiMesh;

namespace Flare
{
	namespace Math
	{
		struct AffineTransform;
	}

	struct AssetMetadata;
	struct MeshImportSettings;
	struct SceneData;

	class Material;
	class Prefab;

	class MeshHierarchyImporter
	{
	public:
		enum class NodeType
		{
			Empty,
			Mesh,
			PointLight,
			SpotLight,
		};

		MeshHierarchyImporter(const aiScene& scene,
			const SceneData& sceneData,
			const AssetMetadata& assetMetadata,
			const std::unordered_map<uint32_t, Ref<Material>>& materials,
			MeshImportSettings& importSettings);

		void Import();

		inline Ref<Prefab> GetImportedPrefab() const { return m_Prefab; }
	private:
		NodeType GetNodeType(const aiNode& node) const;
		void VisitNode(const aiNode& node, size_t parentIndex);

		void CopyComponentData(const aiNode& node);

		void CopyTransform(const aiNode& node, const Math::AffineTransform& parentTransform);
		void CopyMeshComponent(const aiNode& node);
		void CopyLightData(const aiNode& node);
	private:
		const aiScene& m_Scene;
		const SceneData& m_SceneData;
		const std::unordered_map<uint32_t, Ref<Material>>& m_Materials;
		MeshImportSettings& m_ImportSettings;

		std::unordered_map<const aiNode*, size_t> m_NodeIndexMap;
		std::unordered_map<std::string_view, uint32_t> m_LightNameToIndex;

		ArchetypeId m_DefaultNodeArchetype = INVALID_ARCHETYPE_ID;
		ArchetypeId m_DefaultRootArchetype = INVALID_ARCHETYPE_ID;
		ArchetypeId m_PointLightArchetype = INVALID_ARCHETYPE_ID;
		ArchetypeId m_SpotLightArchetype = INVALID_ARCHETYPE_ID;

		Ref<Prefab> m_Prefab = nullptr;
		const AssetMetadata& m_AssetMetadata;
	};

}
