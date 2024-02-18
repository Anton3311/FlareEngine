#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/VertexArray.h"
#include "Flare/Renderer/Buffer.h"
#include "Flare/Math/Math.h"

#include "FlareCore/Collections/Span.h"
#include "FlareCore/Serialization/Metadata.h"

namespace Flare
{
	struct SubMesh
	{
		Math::AABB Bounds;

		uint32_t BaseIndex = 0;
		uint32_t IndicesCount = 0;
		uint32_t BaseVertex = 0;
	};

	enum class MeshRenderFlags : uint8_t
	{
		None = 0,
		DontCastShadows = 1,
	};

	FLARE_IMPL_ENUM_BITFIELD(MeshRenderFlags);

	enum class MeshTopology
	{
		Triangles,
	};

	class FLARE_API Mesh : public Asset
	{
	public:
		FLARE_SERIALIZABLE;
		FLARE_ASSET;

		Mesh(MeshTopology topologyType,
			size_t vertexBufferSize,
			IndexBuffer::IndexFormat indexFormat,
			size_t indexBufferSize);

		void AddSubMesh(const Span<glm::vec3>& vertices,
			const MemorySpan& indices,
			const Span<glm::vec3>& normals,
			const Span<glm::vec2>& uvs);

		constexpr size_t GetVertexBufferSize() const { return m_VertexBufferSize; }
		constexpr size_t GetIndexBufferSize() const { return m_IndexBufferSize; }

		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		inline MeshTopology GetTopologyType() const { return m_TopologyType; }

		inline const Ref<VertexArray>& GetVertexArray() const { return m_VertexArray; }
	private:
		IndexBuffer::IndexFormat m_IndexFormat;

		size_t m_VertexBufferSize = 0;
		size_t m_IndexBufferSize = 0;

		size_t m_VertexBufferOffset = 0;
		size_t m_IndexBufferOffset = 0;

		Ref<VertexArray> m_VertexArray = nullptr;
		Ref<IndexBuffer> m_IndexBuffer = nullptr;
		Ref<VertexBuffer> m_Vertices = nullptr;
		Ref<VertexBuffer> m_Normals = nullptr;
		Ref<VertexBuffer> m_UVs = nullptr;

		MeshTopology m_TopologyType;
		std::vector<SubMesh> m_SubMeshes;
	};
}