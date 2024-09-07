#include "Mesh.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Platform/Vulkan/VulkanBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	SharedMesh::SharedMesh(size_t vertexCount, IndexFormat indexFormat, size_t indexCount)
		: m_VertexCount(vertexCount), m_IndexCount(indexCount), m_IndexFormat(indexFormat)
	{
		FLARE_PROFILE_FUNCTION();
		Vertices = GPUBuffer::CreateVertexBuffer(sizeof(glm::vec3) * m_VertexCount, GPUBufferMemoryType::Static);
		Normals = GPUBuffer::CreateVertexBuffer(sizeof(glm::vec3) * m_VertexCount, GPUBufferMemoryType::Static);
		Tangents = GPUBuffer::CreateVertexBuffer(sizeof(glm::vec3) * m_VertexCount, GPUBufferMemoryType::Static);
		UVs = GPUBuffer::CreateVertexBuffer(sizeof(glm::vec2) * m_VertexCount, GPUBufferMemoryType::Static);

		IndexBuffer = GPUBuffer::CreateIndexBuffer(indexCount, indexFormat, GPUBufferMemoryType::Static);
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

	Mesh::Mesh(size_t vertexBufferSize, IndexFormat indexFormat, size_t indexBufferSize)
		: Asset(AssetType::Mesh),
		m_VertexCount(vertexBufferSize),
		m_IndexFormat(indexFormat),
		m_IndexCount(indexBufferSize)
	{
		// TODO: Remove this constructor
	}

	Mesh::Mesh(MemorySpan indices,
		IndexFormat indexFormat,
		Span<const glm::vec3> vertices,
		Span<const glm::vec3> normals,
		Span<const glm::vec3> tangents,
		Span<const glm::vec2> uvs)
		: Asset(AssetType::Mesh),
		m_IndexFormat(indexFormat),
		m_VertexBufferOffset(0),
		m_IndexBufferOffset(0)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(vertices.GetSize() == normals.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == tangents.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == uvs.GetSize());

		CreateBuffers(indices, indexFormat, vertices, normals, tangents, uvs);

		SubMesh& subMesh = m_SubMeshes.emplace_back();
		subMesh.BaseIndex = 0;
		subMesh.BaseVertex = 0;
		subMesh.IndicesCount = (uint32_t)m_IndexCount;
		subMesh.Bounds.Min = vertices[0];
		subMesh.Bounds.Max = vertices[1];

		for (glm::vec3 vertex : vertices)
		{
			subMesh.Bounds.Min = glm::min(vertex, subMesh.Bounds.Min);
			subMesh.Bounds.Max = glm::min(vertex, subMesh.Bounds.Max);
		}
	}

	Mesh::Mesh(MemorySpan indices,
		IndexFormat indexFormat,
		Span<const glm::vec3> vertices,
		Span<const glm::vec3> normals,
		Span<const glm::vec3> tangents,
		Span<const glm::vec2> uvs,
		Span<const SubMesh>& subMeshes)
		: Asset(AssetType::Mesh),
		m_IndexFormat(indexFormat),
		m_VertexCount(vertices.GetSize()),
		m_VertexBufferOffset(0),
		m_IndexBufferOffset(0),
		m_SubMeshes(subMeshes.begin(), subMeshes.end())
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(vertices.GetSize() == normals.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == tangents.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == uvs.GetSize());

		CreateBuffers(indices, indexFormat, vertices, normals, tangents, uvs);

		m_Bounds = m_SubMeshes[0].Bounds;

		for (const SubMesh& subMesh : m_SubMeshes)
		{
			m_Bounds.Min = glm::min(m_Bounds.Min, subMesh.Bounds.Min);
			m_Bounds.Max = glm::max(m_Bounds.Max, subMesh.Bounds.Max);
		}
	}

	Mesh::Mesh(Ref<SharedMesh> sharedMesh,
		MemorySpan indices,
		Span<const glm::vec3> vertices,
		Span<const glm::vec3> normals,
		Span<const glm::vec3> tangents,
		Span<const glm::vec2> uvs)
		: Asset(AssetType::Mesh), m_IndexFormat(sharedMesh->GetIndexFormat()), m_SharedMesh(sharedMesh)
	{
		FLARE_CORE_ASSERT(vertices.GetSize() == normals.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == tangents.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == uvs.GetSize());

		FLARE_PROFILE_FUNCTION();

		m_VertexCount = vertices.GetSize();
		m_IndexCount = indices.GetSize() / GetIndexFormatSize(sharedMesh->GetIndexFormat());
		SharedMesh::MeshOffset subAllocation = sharedMesh->AllocateMesh(m_VertexCount, m_IndexCount);

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
		: Asset(AssetType::Mesh), m_IndexFormat(sharedMesh->GetIndexFormat()), m_SubMeshes(subMeshes), m_SharedMesh(sharedMesh)
	{
		FLARE_CORE_ASSERT(m_SubMeshes.size() > 0);

		m_IndexBuffer = sharedMesh->IndexBuffer;
		m_Vertices = sharedMesh->Vertices;
		m_Normals = sharedMesh->Normals;
		m_Tangents = sharedMesh->Tangents;
		m_UVs = sharedMesh->UVs;

		m_Bounds = m_SubMeshes[0].Bounds;

		m_IndexBufferOffset = 0;
		m_VertexBufferOffset = 0;

		for (const SubMesh& subMesh : m_SubMeshes)
		{
			m_Bounds.Min = glm::min(m_Bounds.Min, subMesh.Bounds.Min);
			m_Bounds.Max = glm::max(m_Bounds.Max, subMesh.Bounds.Max);

			m_IndexBufferOffset = glm::min(m_IndexBufferOffset, (size_t)subMesh.BaseIndex);
			m_VertexBufferOffset = glm::min(m_VertexBufferOffset, (size_t)subMesh.BaseVertex);
		}
	}

	Mesh::~Mesh()
	{
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

	void Mesh::CreateBuffers(MemorySpan indices,
		IndexFormat indexFormat,
		Span<const glm::vec3> vertices,
		Span<const glm::vec3> normals,
		Span<const glm::vec3> tangents,
		Span<const glm::vec2> uvs)
	{
		FLARE_PROFILE_FUNCTION();
		Ref<CommandBuffer> commandBuffer = VulkanContext::GetInstance().GetUploadCommandBuffer();

		m_VertexCount = vertices.GetSize();
		m_IndexCount = indices.GetSize() / GetIndexFormatSize(indexFormat);

		GPUBufferSpecifications vertexBufferSpecifications{};
		vertexBufferSpecifications.MemoryType = GPUBufferMemoryType::Static;
		vertexBufferSpecifications.Size = sizeof(glm::vec3) * m_VertexCount;
		vertexBufferSpecifications.Usage = GPUBufferUsage::VertexBuffer;

		GPUBufferSpecifications uvBufferSpecifications{};
		uvBufferSpecifications.MemoryType = GPUBufferMemoryType::Static;
		uvBufferSpecifications.Size = sizeof(glm::vec2) * m_VertexCount;
		uvBufferSpecifications.Usage = GPUBufferUsage::VertexBuffer;

		m_Vertices = GPUBuffer::Create(vertexBufferSpecifications);
		m_Normals = GPUBuffer::Create(vertexBufferSpecifications);
		m_Tangents = GPUBuffer::Create(vertexBufferSpecifications);
		m_UVs = GPUBuffer::Create(uvBufferSpecifications);

		m_IndexBuffer = GPUBuffer::CreateIndexBuffer(m_IndexCount, m_IndexFormat, GPUBufferMemoryType::Static);

		m_Vertices->SetData(MemorySpan(vertices.GetData(), m_VertexCount), 0, commandBuffer);
		m_Normals->SetData(MemorySpan(normals.GetData(), m_VertexCount), 0, commandBuffer);
		m_Tangents->SetData(MemorySpan(tangents.GetData(), m_VertexCount), 0, commandBuffer);
		m_UVs->SetData(MemorySpan(uvs.GetData(), m_VertexCount), 0, commandBuffer);

		m_IndexBuffer->SetData(indices, 0, commandBuffer);
	}

	Ref<Mesh> Mesh::Create(size_t vertexBufferSize, IndexFormat indexFormat, size_t indexBufferSize)
	{
		FLARE_PROFILE_FUNCTION();

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return Ref<Mesh>::New(vertexBufferSize, indexFormat, indexBufferSize);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<Mesh> Mesh::Create(MemorySpan indices,
		IndexFormat indexFormat,
		Span<const glm::vec3> vertices,
		Span<const glm::vec3> normals,
		Span<const glm::vec3> tangents,
		Span<const glm::vec2> uvs)
	{
		FLARE_PROFILE_FUNCTION();

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return Ref<Mesh>::New(indices, indexFormat, vertices, normals, tangents, uvs);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}
}
