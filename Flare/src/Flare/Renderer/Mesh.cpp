#include "Mesh.h"

namespace Flare
{
	Mesh::Mesh(MeshTopology topologyType, size_t vertexBufferSize, IndexBuffer::IndexFormat indexFormat, size_t indexBufferSize)
		: Asset(AssetType::Mesh),
		m_TopologyType(topologyType),
		m_VertexBufferSize(vertexBufferSize),
		m_IndexFormat(indexFormat),
		m_IndexBufferSize(indexBufferSize) {}

	void Mesh::AddSubMesh(const Span<glm::vec3>& vertices,
		const MemorySpan& indices,
		const Span<glm::vec3>& normals,
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

		if (!m_VertexArray)
			m_VertexArray = VertexArray::Create();

		if (!m_IndexBuffer)
		{
			m_IndexBuffer = IndexBuffer::Create(m_IndexFormat, m_IndexBufferSize);
			m_VertexArray->SetIndexBuffer(m_IndexBuffer);
		}

		if (!m_Vertices)
		{
			m_Vertices = VertexBuffer::Create(m_VertexBufferSize * sizeof(glm::vec3));
			m_Vertices->SetLayout({ { "i_Position", ShaderDataType::Float3 } });
			m_VertexArray->AddVertexBuffer(m_Vertices);
		}

		if (!m_Normals)
		{
			m_Normals = VertexBuffer::Create(m_VertexBufferSize * sizeof(glm::vec3));
			m_Normals->SetLayout({ { "i_Normal", ShaderDataType::Float3 } });
			m_VertexArray->AddVertexBuffer(m_Normals);
		}

		if (!m_UVs)
		{
			m_UVs = VertexBuffer::Create(m_VertexBufferSize * sizeof(glm::vec2));
			m_UVs->SetLayout({ { "i_UV", ShaderDataType::Float2 } });
			m_VertexArray->AddVertexBuffer(m_UVs);
		}

		m_Vertices->SetData(vertices.GetData(), vertices.GetSize() * sizeof(glm::vec3), m_VertexBufferOffset * sizeof(glm::vec3));
		m_Normals->SetData(normals.GetData(), normals.GetSize() * sizeof(glm::vec3), m_VertexBufferOffset * sizeof(glm::vec3));
		m_UVs->SetData(uvs.GetData(), uvs.GetSize() * sizeof(glm::vec2), m_VertexBufferOffset * sizeof(glm::vec2));

		m_IndexBuffer->SetData(indices, m_IndexBufferOffset);

		size_t indicesCount = indices.GetSize() / indexSize;

		subMesh.BaseIndex = m_IndexBufferOffset / indexSize;
		subMesh.BaseVertex = m_VertexBufferOffset;
		subMesh.IndicesCount = indicesCount;

		m_VertexBufferOffset += vertices.GetSize();
		m_IndexBufferOffset += indices.GetSize();
	}
}
