#pragma once

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/Mesh.h"

#include <glm/glm.hpp>
#include <vector>

struct aiScene;
struct aiNode;
struct aiMesh;

namespace Flare
{
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
		std::vector<uint32_t> UsedMaterials;
	};

	class StaticMeshImporter
	{
	public:
		StaticMeshImporter(const aiScene* scene)
			: m_Scene(scene) {}

		void Import();

		inline const SceneData& GetSceneData() const { return m_SceneData; }
	private:
		void WalkHierarchy(const aiNode* node, const glm::mat4& parentTransform);
		void VisitNode(const aiNode* node, const glm::mat4& parentTransform);

		void CopySubMeshData(const aiMesh* node);
		void FlattenHierarchy(const aiNode* node, const glm::mat4& transform, size_t subMeshStart, size_t subMeshEnd);

		void ReserveBuffers();
		void CountVerticesAndIndicesRecursively(const aiNode* node, size_t& vertexCount, size_t& indexCount);
		void CountVerticesAndIndices(const aiNode* node, size_t& vertexCount, size_t& indexCount);
	private:
		size_t m_VertexOffset = 0;
		size_t m_IndexOffset = 0;

		SceneData m_SceneData;
		const aiScene* m_Scene = nullptr;
	};
}
