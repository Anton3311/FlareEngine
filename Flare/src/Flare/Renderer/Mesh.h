#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/VertexArray.h"
#include "Flare/Renderer/Buffer.h"
#include "Flare/Math/Math.h"

#include "FlareCore/Collections/Span.h"

namespace Flare
{
	struct SubMesh
	{
		Math::AABB Bounds;

		Ref<VertexArray> VertexArray = nullptr;
		Ref<IndexBuffer> Indicies = nullptr;
		Ref<VertexBuffer> Vertices = nullptr;
		Ref<VertexBuffer> Normals = nullptr;
		Ref<VertexBuffer> UVs = nullptr;
	};

	enum class MeshTopology
	{
		Triangles,
	};

	class FLARE_API Mesh : public Asset
	{
	public:
		Mesh(MeshTopology topologyType);

		void AddSubMesh(const Span<glm::vec3>& vertices,
			IndexBuffer::IndexFormat indexFormat,
			const MemorySpan& indices,
			const Span<glm::vec3>* normals = nullptr,
			const Span<glm::vec2>* uvs = nullptr);

		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		inline MeshTopology GetTopologyType() const { return m_TopologyType; }
	private:
		void InitializeLastSubMeshBuffers();
	private:
		MeshTopology m_TopologyType;
		Ref<VertexBuffer> m_InstanceBuffer = nullptr;
		std::vector<SubMesh> m_SubMeshes;
	};
}