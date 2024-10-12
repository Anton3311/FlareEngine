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

	class FLARE_API SharedMesh : public RefCounted<SharedMesh>
	{
	public:
		struct MeshOffset
		{
			size_t VertexOffset = 0;
			size_t IndexOffset = 0;
		};

		SharedMesh(size_t vertexCount, IndexFormat indexFormat, size_t indexCount);

		MeshOffset AllocateMesh(size_t vertexCount, size_t indexCount);

		inline IndexFormat GetIndexFormat() const { return m_IndexFormat; }
	public:
		Ref<GPUBuffer> IndexBuffer = nullptr;
		Ref<GPUBuffer> Vertices = nullptr;
		Ref<GPUBuffer> Normals = nullptr;
		Ref<GPUBuffer> Tangents = nullptr;
		Ref<GPUBuffer> UVs = nullptr;
	private:
		IndexFormat m_IndexFormat;

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
			IndexFormat indexFormat,
			size_t indexBufferSize);

		Mesh(MemorySpan indices,
			IndexFormat indexFormat,
			Span<const glm::vec3> vertices,
			Span<const glm::vec3> normals,
			Span<const glm::vec3> tangents,
			Span<const glm::vec2> uvs);

		Mesh(MemorySpan indices,
			IndexFormat indexFormat,
			Span<const glm::vec3> vertices,
			Span<const glm::vec3> normals,
			Span<const glm::vec3> tangents,
			Span<const glm::vec2> uvs,
			Span<const SubMesh> subMeshes);

		Mesh(Ref<SharedMesh> sharedMesh,
			MemorySpan indices,
			Span<const glm::vec3> vertices,
			Span<const glm::vec3> normals,
			Span<const glm::vec3> tangents,
			Span<const glm::vec2> uvs);

		Mesh(Ref<SharedMesh> sharedMesh, std::vector<SubMesh>&& subMeshes);

		~Mesh();

		void SetDebugName(std::string_view debugName);
		inline const std::string& GetDebugName() const { return m_DebugName; }

		constexpr size_t GetVertexCount() const { return m_VertexCount; }
		constexpr size_t GetIndexCount() const { return m_IndexCount; }

		inline Ref<GPUBuffer> GetIndexBuffer() const { return m_IndexBuffer; }
		inline Ref<GPUBuffer> GetVertices() const { return m_Vertices; }
		inline Ref<GPUBuffer> GetNormals() const { return m_Normals; }
		inline Ref<GPUBuffer> GetTangents() const { return m_Tangents; }
		inline Ref<GPUBuffer> GetUVs() const { return m_UVs; }

		inline const Math::AABB& GetBounds() const { return m_Bounds; }

		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
		inline IndexFormat GetIndexFormat() const { return m_IndexFormat; }

		inline Ref<SharedMesh> GetSharedMesh() const { return m_SharedMesh; }

		inline SubMesh GetFullMeshRange() const
		{
			SubMesh fullMesh{};
			fullMesh.BaseIndex = (uint32_t)m_VertexBufferOffset;
			fullMesh.BaseVertex = (uint32_t)m_IndexBufferOffset;
			fullMesh.Bounds = m_Bounds;
			fullMesh.IndicesCount = (uint32_t)m_IndexCount;
			return fullMesh;
		}
	private:
		void UpdateBufferDebugNames();
		void CreateBuffers(MemorySpan indices,
			IndexFormat indexFormat,
			Span<const glm::vec3> vertices,
			Span<const glm::vec3> normals,
			Span<const glm::vec3> tangents,
			Span<const glm::vec2> uvs);
	public:
		static Ref<Mesh> Create( size_t vertexBufferSize, IndexFormat indexFormat, size_t indexBufferSize);

		static Ref<Mesh> Create(MemorySpan indices,
			IndexFormat indexFormat,
			Span<const glm::vec3> vertices,
			Span<const glm::vec3> normals,
			Span<const glm::vec3> tangents,
			Span<const glm::vec2> uvs);
	protected:
		std::string m_DebugName;
		IndexFormat m_IndexFormat;

		Ref<SharedMesh> m_SharedMesh = nullptr;

		Math::AABB m_Bounds;

		size_t m_VertexCount = 0;
		size_t m_IndexCount = 0;

		size_t m_VertexBufferOffset = 0;
		size_t m_IndexBufferOffset = 0;

		Ref<GPUBuffer> m_IndexBuffer = nullptr;
		Ref<GPUBuffer> m_Vertices = nullptr;
		Ref<GPUBuffer> m_Normals = nullptr;
		Ref<GPUBuffer> m_Tangents = nullptr;
		Ref<GPUBuffer> m_UVs = nullptr;

		std::vector<SubMesh> m_SubMeshes;

		friend class SharedMesh;
	};
}