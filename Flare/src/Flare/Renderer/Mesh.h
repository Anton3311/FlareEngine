#pragma once

#include "Flare/AssetManager/Asset.h"
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

	class FLARE_API Mesh : public Asset
	{
	public:
		FLARE_SERIALIZABLE;
		FLARE_ASSET;

		Mesh(size_t vertexBufferSize,
			IndexBuffer::IndexFormat indexFormat,
			size_t indexBufferSize);

		Mesh(MemorySpan indices,
			IndexBuffer::IndexFormat indexFormat,
			Span<glm::vec3> vertices,
			Span<glm::vec3> normals,
			Span<glm::vec3> tangents,
			Span<glm::vec2> uvs);

		~Mesh();

		virtual void AddSubMesh(const Span<glm::vec3>& vertices,
			const MemorySpan& indices,
			const Span<glm::vec3>& normals,
			const Span<glm::vec3>& tangents,
			const Span<glm::vec2>& uvs);

		void AddSubMesh(const SubMesh& subMesh);

		constexpr size_t GetVertexBufferSize() const { return m_VertexBufferSize; }
		constexpr size_t GetIndexBufferSize() const { return m_IndexBufferSize; }

		inline Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		inline Ref<VertexBuffer> GetVertices() const { return m_Vertices; }
		inline Ref<VertexBuffer> GetNormals() const { return m_Normals; }
		inline Ref<VertexBuffer> GetTangents() const { return m_Tangents; }
		inline Ref<VertexBuffer> GetUVs() const { return m_UVs; }

		inline const Math::AABB& GetBounds() const { return m_Bounds; }

		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		inline IndexBuffer::IndexFormat GetIndexFormat() const { return m_IndexFormat; }
	public:
		static Ref<Mesh> Create( size_t vertexBufferSize, IndexBuffer::IndexFormat indexFormat, size_t indexBufferSize);

		static Ref<Mesh> Create(MemorySpan indices,
			IndexBuffer::IndexFormat indexFormat,
			Span<glm::vec3> vertices,
			Span<glm::vec3> normals,
			Span<glm::vec3> tangents,
			Span<glm::vec2> uvs);
	protected:
		IndexBuffer::IndexFormat m_IndexFormat;

		Math::AABB m_Bounds;

		size_t m_VertexBufferSize = 0;
		size_t m_IndexBufferSize = 0;

		size_t m_VertexBufferOffset = 0;
		size_t m_IndexBufferOffset = 0;

		Ref<IndexBuffer> m_IndexBuffer = nullptr;
		Ref<VertexBuffer> m_Vertices = nullptr;
		Ref<VertexBuffer> m_Normals = nullptr;
		Ref<VertexBuffer> m_Tangents = nullptr;
		Ref<VertexBuffer> m_UVs = nullptr;

		std::vector<SubMesh> m_SubMeshes;
	};
}