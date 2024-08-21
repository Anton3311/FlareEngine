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

	class FLARE_API SharedMesh
	{
	public:
		struct MeshOffset
		{
			size_t VertexOffset = 0;
			size_t IndexOffset = 0;
		};

		SharedMesh(size_t vertexCount, IndexBuffer::IndexFormat indexFormat, size_t indexCount);

		MeshOffset AllocateMesh(size_t vertexCount, size_t indexCount);

		inline IndexBuffer::IndexFormat GetIndexFormat() const { return IndexBuffer->GetIndexFormat(); }
	public:
		Ref<IndexBuffer> IndexBuffer = nullptr;
		Ref<VertexBuffer> Vertices = nullptr;
		Ref<VertexBuffer> Normals = nullptr;
		Ref<VertexBuffer> Tangents = nullptr;
		Ref<VertexBuffer> UVs = nullptr;
	private:
		size_t m_VertexCount = 0;
		size_t m_IndexCount = 0;

		size_t m_VertexOffset = 0;
		size_t m_IndexOffset = 0;
	};

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
			Span<const glm::vec3> vertices,
			Span<const glm::vec3> normals,
			Span<const glm::vec3> tangents,
			Span<const glm::vec2> uvs);

		Mesh(Ref<SharedMesh> sharedMesh,
			MemorySpan indices,
			Span<const glm::vec3> vertices,
			Span<const glm::vec3> normals,
			Span<const glm::vec3> tangents,
			Span<const glm::vec2> uvs);

		Mesh(Ref<SharedMesh> sharedMesh, std::vector<SubMesh>&& subMeshes);

		~Mesh();

		void AddSubMesh(const SubMesh& subMesh);

		void SetDebugName(std::string_view debugName);
		inline const std::string& GetDebugName() const { return m_DebugName; }

		constexpr size_t GetVertexBufferSize() const { return m_VertexBufferSize; }
		constexpr size_t GetIndexBufferSize() const { return m_IndexBufferSize; }

		inline size_t GetIndexCount() const { return m_IndexBuffer->GetCount(); }

		inline Ref<IndexBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		inline Ref<VertexBuffer> GetVertices() const { return m_Vertices; }
		inline Ref<VertexBuffer> GetNormals() const { return m_Normals; }
		inline Ref<VertexBuffer> GetTangents() const { return m_Tangents; }
		inline Ref<VertexBuffer> GetUVs() const { return m_UVs; }

		inline const Math::AABB& GetBounds() const { return m_Bounds; }

		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		inline IndexBuffer::IndexFormat GetIndexFormat() const { return m_IndexFormat; }

		inline Ref<SharedMesh> GetSharedMesh() const { return m_SharedMesh; }
	private:
		void UpdateBufferDebugNames();
	public:
		static Ref<Mesh> Create( size_t vertexBufferSize, IndexBuffer::IndexFormat indexFormat, size_t indexBufferSize);

		static Ref<Mesh> Create(MemorySpan indices,
			IndexBuffer::IndexFormat indexFormat,
			Span<const glm::vec3> vertices,
			Span<const glm::vec3> normals,
			Span<const glm::vec3> tangents,
			Span<const glm::vec2> uvs);
	protected:
		std::string m_DebugName;
		IndexBuffer::IndexFormat m_IndexFormat;

		Ref<SharedMesh> m_SharedMesh = nullptr;

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

		friend class SharedMesh;
	};
}