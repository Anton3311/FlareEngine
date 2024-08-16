#include "Mesh.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Flare/Platform/Vulkan/VulkanIndexBuffer.h"

namespace Flare
{
	SharedMesh::SharedMesh(size_t vertexCount, IndexBuffer::IndexFormat indexFormat, size_t indexCount)
		: m_VertexCount(vertexCount), m_IndexCount(indexCount)
	{
		FLARE_PROFILE_FUNCTION();
		Vertices = VertexBuffer::Create(sizeof(glm::vec3) * m_VertexCount, GPUBufferUsage::Static);
		Normals = VertexBuffer::Create(sizeof(glm::vec3) * m_VertexCount, GPUBufferUsage::Static);
		Tangents = VertexBuffer::Create(sizeof(glm::vec3) * m_VertexCount, GPUBufferUsage::Static);
		UVs = VertexBuffer::Create(sizeof(glm::vec2) * m_VertexCount, GPUBufferUsage::Static);

		IndexBuffer = IndexBuffer::Create(indexFormat, indexCount, GPUBufferUsage::Static);

		As<VulkanVertexBuffer>(Vertices)->GetBuffer().EnsureAllocated();
		As<VulkanVertexBuffer>(Normals)->GetBuffer().EnsureAllocated();
		As<VulkanVertexBuffer>(Tangents)->GetBuffer().EnsureAllocated();
		As<VulkanVertexBuffer>(UVs)->GetBuffer().EnsureAllocated();
		As<VulkanIndexBuffer>(IndexBuffer)->GetBuffer().EnsureAllocated();
	}

	SharedMesh::MeshOffset SharedMesh::AllocateMesh(size_t vertexCount, size_t indexCount)
	{
		FLARE_CORE_ASSERT(m_VertexOffset + vertexCount <= m_VertexCount);
		FLARE_CORE_ASSERT(m_IndexOffset + indexCount <= m_IndexCount);

		MeshOffset meshOffset{};
		meshOffset.VertexOffset = m_VertexOffset;
		meshOffset.IndexOffset = m_IndexOffset;

		m_VertexOffset += vertexCount;
		m_IndexOffset += indexCount;

		return meshOffset;
	}

	//
	// Mesh
	//

	FLARE_SERIALIZABLE_IMPL(Mesh);
	FLARE_IMPL_ASSET(Mesh);

	Mesh::Mesh(size_t vertexBufferSize, IndexBuffer::IndexFormat indexFormat, size_t indexBufferSize)
		: Asset(AssetType::Mesh),
		m_VertexBufferSize(vertexBufferSize),
		m_IndexFormat(indexFormat),
		m_IndexBufferSize(indexBufferSize)
	{
	}

	Mesh::Mesh(MemorySpan indices,
		IndexBuffer::IndexFormat indexFormat,
		Span<const glm::vec3> vertices,
		Span<const glm::vec3> normals,
		Span<const glm::vec3> tangents,
		Span<const glm::vec2> uvs)
		: Asset(AssetType::Mesh),
		m_IndexFormat(indexFormat),
		m_VertexBufferSize(vertices.GetSize()),
		m_VertexBufferOffset(0),
		m_IndexBufferSize(indices.GetSize()),
		m_IndexBufferOffset(0)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(vertices.GetSize() == normals.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == tangents.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == uvs.GetSize());

		Ref<CommandBuffer> commandBuffer = VulkanContext::GetInstance().GetUploadCommandBuffer();

		m_Vertices = VertexBuffer::Create(sizeof(glm::vec3) * vertices.GetSize(), vertices.GetData(), commandBuffer);
		m_Normals = VertexBuffer::Create(sizeof(glm::vec3) * normals.GetSize(), normals.GetData(), commandBuffer);
		m_Tangents = VertexBuffer::Create(sizeof(glm::vec3) * tangents.GetSize(), tangents.GetData(), commandBuffer);
		m_UVs = VertexBuffer::Create(sizeof(glm::vec2) * uvs.GetSize(), uvs.GetData(), commandBuffer);

		m_IndexBuffer = IndexBuffer::Create(m_IndexFormat, indices, commandBuffer);

		SubMesh& subMesh = m_SubMeshes.emplace_back();
		subMesh.BaseIndex = 0;
		subMesh.BaseVertex = 0;
		subMesh.IndicesCount = m_IndexBuffer->GetCount();
		subMesh.Bounds.Min = vertices[0];
		subMesh.Bounds.Max = vertices[1];

		for (glm::vec3 vertex : vertices)
		{
			subMesh.Bounds.Min = glm::min(vertex, subMesh.Bounds.Min);
			subMesh.Bounds.Max = glm::min(vertex, subMesh.Bounds.Max);
		}
	}

	Mesh::Mesh(Ref<SharedMesh> sharedMesh,
		MemorySpan indices,
		Span<const glm::vec3> vertices,
		Span<const glm::vec3> normals,
		Span<const glm::vec3> tangents,
		Span<const glm::vec2> uvs)
		: Asset(AssetType::Mesh), m_IndexFormat(sharedMesh->GetIndexFormat())
	{
		FLARE_CORE_ASSERT(vertices.GetSize() == normals.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == tangents.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == uvs.GetSize());

		FLARE_PROFILE_FUNCTION();

		m_VertexBufferSize = vertices.GetSize();
		m_IndexBufferSize = indices.GetSize() / IndexBuffer::GetIndexFormatSize(sharedMesh->GetIndexFormat());
		SharedMesh::MeshOffset subAllocation = sharedMesh->AllocateMesh(m_VertexBufferSize, m_IndexBufferSize);

		m_VertexBufferOffset = subAllocation.VertexOffset;
		m_IndexBufferOffset = subAllocation.IndexOffset;

		Ref<CommandBuffer> commandBuffer = VulkanContext::GetInstance().GetUploadCommandBuffer();

		m_IndexBuffer = sharedMesh->IndexBuffer;
		m_Vertices = sharedMesh->Vertices;
		m_Normals = sharedMesh->Normals;
		m_Tangents = sharedMesh->Tangents;
		m_UVs = sharedMesh->UVs;

		m_IndexBuffer->SetData(indices, m_IndexBufferOffset, commandBuffer);

		m_Vertices->SetData(MemorySpan(vertices.GetData(), vertices.GetSize()), m_VertexBufferOffset, commandBuffer);
		m_Normals->SetData(MemorySpan(normals.GetData(), normals.GetSize()), m_VertexBufferOffset, commandBuffer);
		m_Tangents->SetData(MemorySpan(tangents.GetData(), tangents.GetSize()), m_VertexBufferOffset, commandBuffer);
		m_UVs->SetData(MemorySpan(uvs.GetData(), uvs.GetSize()), m_VertexBufferOffset, commandBuffer);
	}

	Mesh::Mesh(Ref<SharedMesh> sharedMesh, std::vector<SubMesh>&& subMeshes)
		: Asset(AssetType::Mesh), m_IndexFormat(sharedMesh->GetIndexFormat()), m_SubMeshes(subMeshes)
	{
		m_IndexBuffer = sharedMesh->IndexBuffer;
		m_Vertices = sharedMesh->Vertices;
		m_Normals = sharedMesh->Normals;
		m_Tangents = sharedMesh->Tangents;
		m_UVs = sharedMesh->UVs;
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::AddSubMesh(const SubMesh& subMesh)
	{
		if (m_SubMeshes.size() == 0)
		{
			m_Bounds = subMesh.Bounds;
		}
		else
		{
			m_Bounds.Min = glm::min(subMesh.Bounds.Min, m_Bounds.Min);
			m_Bounds.Max = glm::max(subMesh.Bounds.Max, m_Bounds.Max);
		}

		m_SubMeshes.push_back(subMesh);
	}

	void Mesh::SetDebugName(std::string_view debugName)
	{
		FLARE_PROFILE_FUNCTION();
		m_DebugName = debugName;

		UpdateBufferDebugNames();
	}

	void Mesh::UpdateBufferDebugNames()
	{
		FLARE_PROFILE_FUNCTION();

		if (m_DebugName.empty())
			return;

		m_Vertices->SetDebugName(fmt::format("{}.Vertices", m_DebugName));
		m_Normals->SetDebugName(fmt::format("{}.Normals", m_DebugName));
		m_IndexBuffer->SetDebugName(fmt::format("{}.Indices", m_DebugName));
		m_Tangents->SetDebugName(fmt::format("{}.Tangents", m_DebugName));
		m_UVs->SetDebugName(fmt::format("{}.UVs", m_DebugName));
	}

	Ref<Mesh> Mesh::Create(size_t vertexBufferSize, IndexBuffer::IndexFormat indexFormat, size_t indexBufferSize)
	{
		FLARE_PROFILE_FUNCTION();

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<Mesh>(vertexBufferSize, indexFormat, indexBufferSize);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<Mesh> Mesh::Create(MemorySpan indices,
		IndexBuffer::IndexFormat indexFormat,
		Span<const glm::vec3> vertices,
		Span<const glm::vec3> normals,
		Span<const glm::vec3> tangents,
		Span<const glm::vec2> uvs)
	{
		FLARE_PROFILE_FUNCTION();

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<Mesh>(indices, indexFormat, vertices, normals, tangents, uvs);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}
}
