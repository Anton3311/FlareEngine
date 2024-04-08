#include "OpenGLMesh.h"

namespace Flare
{
	OpenGLMesh::OpenGLMesh(MeshTopology topologyType, size_t vertexBufferSize, IndexBuffer::IndexFormat indexFormat, size_t indexBufferSize)
		: Mesh(topologyType, vertexBufferSize, indexFormat, indexBufferSize)
	{
	}

	OpenGLMesh::OpenGLMesh(MeshTopology topology,
		MemorySpan indices,
		IndexBuffer::IndexFormat indexFormat,
		Span<glm::vec3> vertices,
		Span<glm::vec3> normals,
		Span<glm::vec3> tangents,
		Span<glm::vec2> uvs)
		: Mesh(topology, indices, indexFormat, vertices, normals, tangents, uvs)
	{
		CreateVertexArray();
	}

	void OpenGLMesh::AddSubMesh(
		const Span<glm::vec3>& vertices,
		const MemorySpan& indices,
		const Span<glm::vec3>& normals,
		const Span<glm::vec3>& tangents,
		const Span<glm::vec2>& uvs)
	{
		bool hasVertexBuffers = m_Vertices != nullptr;
		Mesh::AddSubMesh(vertices, indices, normals, tangents, uvs);

		if (m_Vertices != nullptr && !hasVertexBuffers)
		{
			CreateVertexArray();
		}
	}

	void OpenGLMesh::CreateVertexArray()
	{
		m_VertexArray.SetIndexBuffer(m_IndexBuffer);

		m_VertexArray.AddVertexBuffer(m_Vertices, 0);
		m_VertexArray.AddVertexBuffer(m_Normals, 1);
		m_VertexArray.AddVertexBuffer(m_Tangents, 2);
		m_VertexArray.AddVertexBuffer(m_UVs, 3);

		m_VertexArray.Unbind();
	}
}
