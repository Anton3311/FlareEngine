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

	struct SubMeshesPair
	{
		SubMesh MainSubMesh;
		SubMesh DepthOnlySubMesh;
	};

	glm::mat4 ConvertToColumnMajor(const aiMatrix4x4& matrix);

	struct SceneData
	{
		~SceneData();

		IndexFormat IndexFormat = IndexFormat::UInt32;

		size_t IndexCount = 0;
		size_t VertexCount = 0;
		size_t SubMeshCount = 0;

		uint16_t* Indices16 = nullptr;
		uint32_t* Indices32 = nullptr;

		uint16_t* DepthOnlyIndices16 = nullptr;
		uint32_t* DepthOnlyIndices32 = nullptr;

		glm::vec3* DepthOnlyVertices = nullptr;
		size_t DepthOnlyVertexCount = 0;

		glm::vec3* VertexDataBuffer = nullptr;

		// All of these, are suballocated from the VertexDataBuffer
		glm::vec3* Vertices = nullptr;
		glm::vec3* Normals = nullptr;
		glm::vec3* Tangents = nullptr;
		glm::vec2* UVs = nullptr;

		size_t MaxSubMeshIndexCount = 0;

		SubMesh* SubMeshes = nullptr;
		SubMesh* DepthOnlySubMeshes = nullptr;
		std::unordered_set<uint32_t> UsedMaterials;

		Ref<SharedMesh> SharedMesh = nullptr;

		std::unordered_map<const aiMesh*, SubMeshesPair> MeshData;
		std::unordered_map<const aiNode*, NodeMesh> NodeToMesh;
	};

	class StaticMeshImporter
	{
	public:
		StaticMeshImporter(const aiScene* scene, const MeshImportSettings& importSettings)
			: m_Scene(scene), m_ImportSettings(importSettings) {}

		~StaticMeshImporter();

		void Import();

		inline const SceneData& GetSceneData() const { return m_SceneData; }
	private:
		void CreateSubMeshes();
		void InitializeSharedMesh();

		void WalkHierarchy(const aiNode* node, const glm::mat4& parentTransform);
		void VisitNode(const aiNode* node, const glm::mat4& parentTransform);

		SubMeshesPair CopySubMeshData(const aiMesh* node);
		void FlattenHierarchy(const aiNode* node, const glm::mat4& transform, size_t subMeshStart, size_t subMeshEnd);

		void InitializeSceneData(size_t vertexCount, size_t indexCount, size_t subMeshCount, IndexFormat indexFormat);
		void CountMeshVerticesAndIndices(size_t& outVertexCount, size_t& outIndexCount);

		void CountVerticesAndIndicesRecursively(const aiNode* node, size_t& vertexCount, size_t& indexCount);
		void CountVerticesAndIndices(const aiNode* node, size_t& vertexCount, size_t& indexCount);
	private:
		const MeshImportSettings& m_ImportSettings;

		size_t m_VertexOffset = 0;
		size_t m_IndexOffset = 0;

		size_t m_InsertedSubMeshCount = 0;

		SceneData m_SceneData;
		const aiScene* m_Scene = nullptr;

		uint32_t* m_TemporaryReducedIndexBuffer = nullptr;
		uint32_t* m_TemporaryUniqueVertexMapping = nullptr;

		size_t m_MaxSubMeshIndexCount = 0;
	};
}
