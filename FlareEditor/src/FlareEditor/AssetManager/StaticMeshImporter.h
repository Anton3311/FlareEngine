#pragma once

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/Mesh.h"

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <assimp/matrix4x4.h>

struct aiScene;
struct aiNode;
struct aiMesh;

namespace Flare
{
	class Prefab;
	struct MeshImportSettings;

	struct NodeMesh
	{
		Ref<Mesh> Mesh = nullptr;
		std::vector<uint32_t> MaterialIndices;
	};

	glm::mat4 ConvertToColumnMajor(const aiMatrix4x4& matrix);

	struct SceneData
	{
		IndexBuffer::IndexFormat IndexFormat = IndexBuffer::IndexFormat::UInt32;

		std::vector<uint16_t> Indices16;
		std::vector<uint32_t> Indices32;

		std::vector<glm::vec3> Vertices;
		std::vector<glm::vec3> Normals;
		std::vector<glm::vec3> Tangents;
		std::vector<glm::vec2> UVs;

		size_t MaxSubMeshIndexCount = 0;

		std::vector<SubMesh> SubMeshes;
		std::unordered_set<uint32_t> UsedMaterials;

		Ref<SharedMesh> SharedMesh = nullptr;
		std::vector<Ref<Mesh>> Meshes;

		std::unordered_map<const aiMesh*, SubMesh> MeshData;
		std::unordered_map<const aiNode*, NodeMesh> NodeToMesh;
	};

	class StaticMeshImporter
	{
	public:
		StaticMeshImporter(const aiScene* scene, const MeshImportSettings& importSettings)
			: m_Scene(scene), m_ImportSettings(importSettings) {}

		void Import();

		inline const SceneData& GetSceneData() const { return m_SceneData; }
	private:
		void CreateSubMeshes();

		void WalkHierarchy(const aiNode* node, const glm::mat4& parentTransform);
		void VisitNode(const aiNode* node, const glm::mat4& parentTransform);

		SubMesh CopySubMeshData(const aiMesh* node);
		void FlattenHierarchy(const aiNode* node, const glm::mat4& transform, size_t subMeshStart, size_t subMeshEnd);

		void ReserveBuffers();
		void CountMeshVerticesAndIndices(size_t& outVertexCount, size_t& outIndexCount);

		void CountVerticesAndIndicesRecursively(const aiNode* node, size_t& vertexCount, size_t& indexCount);
		void CountVerticesAndIndices(const aiNode* node, size_t& vertexCount, size_t& indexCount);
	private:
		const MeshImportSettings& m_ImportSettings;

		size_t m_VertexOffset = 0;
		size_t m_IndexOffset = 0;

		SceneData m_SceneData;
		const aiScene* m_Scene = nullptr;
	};
}
