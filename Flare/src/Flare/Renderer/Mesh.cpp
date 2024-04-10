#include "Mesh.h"

#include "Flare/Renderer/RendererAPI.h"

#include "Flare/Platform/OpenGL/OpenGLMesh.h"

namespace Flare
{
	FLARE_SERIALIZABLE_IMPL(Mesh);
	FLARE_IMPL_ASSET(Mesh);

	Mesh::Mesh(MeshTopology topologyType, size_t vertexBufferSize, IndexBuffer::IndexFormat indexFormat, size_t indexBufferSize)
		: Asset(AssetType::Mesh),
		m_TopologyType(topologyType),
		m_VertexBufferSize(vertexBufferSize),
		m_IndexFormat(indexFormat),
		m_IndexBufferSize(indexBufferSize)
	{
	}

	Mesh::Mesh(MeshTopology topology,
		MemorySpan indices,
		IndexBuffer::IndexFormat indexFormat,
		Span<glm::vec3> vertices,
		Span<glm::vec3> normals,
		Span<glm::vec3> tangents,
		Span<glm::vec2> uvs)
		: Asset(AssetType::Mesh),
		m_TopologyType(topology),
		m_IndexFormat(indexFormat),
		m_VertexBufferSize(vertices.GetSize()),
		m_VertexBufferOffset(0),
		m_IndexBufferSize(indices.GetSize()),
		m_IndexBufferOffset(0)
	{
		FLARE_CORE_ASSERT(vertices.GetSize() == normals.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == tangents.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == uvs.GetSize());

		m_Vertices = VertexBuffer::Create(sizeof(glm::vec3) * vertices.GetSize(), vertices.GetData());
		m_Normals = VertexBuffer::Create(sizeof(glm::vec3) * normals.GetSize(), normals.GetData());
		m_Tangents = VertexBuffer::Create(sizeof(glm::vec3) * tangents.GetSize(), tangents.GetData());
		m_UVs = VertexBuffer::Create(sizeof(glm::vec2) * uvs.GetSize(), uvs.GetData());

		m_IndexBuffer = IndexBuffer::Create(m_IndexFormat, indices);

		m_Vertices->SetLayout({ { "i_Position", ShaderDataType::Float3 } });
		m_Normals->SetLayout({ { "i_Normal", ShaderDataType::Float3 } });
		m_Tangents->SetLayout({ { "i_Tangent", ShaderDataType::Float3 } });
		m_UVs->SetLayout({ { "i_UV", ShaderDataType::Float2 } });
	}

	Mesh::~Mesh()
	{
	}

	void Mesh::AddSubMesh(const Span<glm::vec3>& vertices,
		const MemorySpan& indices,
		const Span<glm::vec3>& normals,
		const Span<glm::vec3>& tangents,
		const Span<glm::vec2>& uvs)
	{
		FLARE_CORE_ASSERT(vertices.GetSize() > 0);
		FLARE_CORE_ASSERT(vertices.GetSize() == normals.GetSize());
		FLARE_CORE_ASSERT(vertices.GetSize() == uvs.GetSize());

		size_t indexSize = 0;
		switch (m_IndexFormat)
		{
		case IndexBuffer::IndexFormat::UInt16:
			indexSize = sizeof(uint16_t);
			break;
		case IndexBuffer::IndexFormat::UInt32:
			indexSize = sizeof(uint32_t);
			break;
		}

		SubMesh& subMesh = m_SubMeshes.emplace_back();
		subMesh.Bounds = Math::AABB(vertices[0], vertices[0]);
		for (size_t i = 1; i < vertices.GetSize(); i++)
		{
			subMesh.Bounds.Min = glm::min(subMesh.Bounds.Min, vertices[i]);
			subMesh.Bounds.Max = glm::max(subMesh.Bounds.Max, vertices[i]);
		}

		if (!m_IndexBuffer)
		{
			m_IndexBuffer = IndexBuffer::Create(m_IndexFormat, m_IndexBufferSize);
		}

		if (!m_Vertices)
		{
			m_Vertices = VertexBuffer::Create(m_VertexBufferSize * sizeof(glm::vec3));
			m_Vertices->SetLayout({ { "i_Position", ShaderDataType::Float3 } });
		}

		if (!m_Normals)
		{
			m_Normals = VertexBuffer::Create(m_VertexBufferSize * sizeof(glm::vec3));
			m_Normals->SetLayout({ { "i_Normal", ShaderDataType::Float3 } });
		}

		if (!m_Tangents)
		{
			m_Tangents = VertexBuffer::Create(m_VertexBufferSize * sizeof(glm::vec3));
			m_Tangents->SetLayout({ { "i_Tangent", ShaderDataType::Float3 } });
		}

		if (!m_UVs)
		{
			m_UVs = VertexBuffer::Create(m_VertexBufferSize * sizeof(glm::vec2));
			m_UVs->SetLayout({ { "i_UV", ShaderDataType::Float2 } });
		}

		m_Vertices->SetData(vertices.GetData(), vertices.GetSize() * sizeof(glm::vec3), m_VertexBufferOffset * sizeof(glm::vec3));
		m_Normals->SetData(normals.GetData(), normals.GetSize() * sizeof(glm::vec3), m_VertexBufferOffset * sizeof(glm::vec3));
		m_Tangents->SetData(tangents.GetData(), tangents.GetSize() * sizeof(glm::vec3), m_VertexBufferOffset * sizeof(glm::vec3));
		m_UVs->SetData(uvs.GetData(), uvs.GetSize() * sizeof(glm::vec2), m_VertexBufferOffset * sizeof(glm::vec2));

		m_IndexBuffer->SetData(indices, m_IndexBufferOffset);

		size_t indicesCount = indices.GetSize() / indexSize;

		subMesh.BaseIndex = (uint32_t)(m_IndexBufferOffset / indexSize);
		subMesh.BaseVertex = (uint32_t)m_VertexBufferOffset;
		subMesh.IndicesCount = (uint32_t)indicesCount;

		m_VertexBufferOffset += vertices.GetSize();
		m_IndexBufferOffset += indices.GetSize();
	}

	void Mesh::AddSubMesh(const SubMesh& subMesh)
	{
		m_SubMeshes.push_back(subMesh);
	}

	Ref<Mesh> Mesh::Create(MeshTopology topology, size_t vertexBufferSize, IndexBuffer::IndexFormat indexFormat, size_t indexBufferSize)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLMesh>(topology, vertexBufferSize, indexFormat, indexBufferSize);
		case RendererAPI::API::Vulkan:
			return CreateRef<Mesh>(topology, vertexBufferSize, indexFormat, indexBufferSize);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<Mesh> Mesh::Create(MeshTopology topology,
		MemorySpan indices,
		IndexBuffer::IndexFormat indexFormat,
		Span<glm::vec3> vertices,
		Span<glm::vec3> normals,
		Span<glm::vec3> tangents,
		Span<glm::vec2> uvs)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLMesh>(topology, indices, indexFormat, vertices, normals, tangents, uvs);
		case RendererAPI::API::Vulkan:
			return CreateRef<Mesh>(topology, indices, indexFormat, vertices, normals, tangents, uvs);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}
}
