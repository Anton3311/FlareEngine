#include "Mesh.h"

namespace Flare
{
	Mesh::Mesh()
		: Asset(AssetType::Mesh)
	{
	}

	Mesh::Mesh(size_t verticesCount, size_t indicesCount)
		: Asset(AssetType::Mesh)
	{
		m_SubMesh.Vertices = VertexBuffer::Create(verticesCount * sizeof(glm::vec3));
		m_SubMesh.Indicies = IndexBuffer::Create(indicesCount);
		m_SubMesh.Normals = VertexBuffer::Create(verticesCount * sizeof(glm::vec3));

		m_SubMesh.MeshVertexArray = VertexArray::Create();
		InitializeBuffers();
	}

	Mesh::Mesh(const glm::vec3* vertices, size_t verticesCount, const uint32_t* indices, size_t indicesCount, const glm::vec3* normals, const glm::vec2* uvs)
		: Asset(AssetType::Mesh)
	{
		m_SubMesh.Vertices = VertexBuffer::Create(verticesCount * sizeof(glm::vec3), vertices);
		m_SubMesh.Indicies = IndexBuffer::Create(indicesCount, indices);
		m_SubMesh.Normals = VertexBuffer::Create(verticesCount * sizeof(glm::vec3), normals);
		m_SubMesh.UVs = VertexBuffer::Create(verticesCount * sizeof(glm::vec2), uvs);

		if (verticesCount > 0)
		{
			m_SubMesh.Bounds = Math::AABB(vertices[0], vertices[0]);

			for (size_t i = 1; i < verticesCount; i++)
			{
				m_SubMesh.Bounds.Min = glm::min(m_SubMesh.Bounds.Min, vertices[i]);
				m_SubMesh.Bounds.Max = glm::max(m_SubMesh.Bounds.Max, vertices[i]);
			}
		}

		m_SubMesh.MeshVertexArray = VertexArray::Create();
		InitializeBuffers();
	}

	void Mesh::SetIndices(const uint32_t* data, size_t count)
	{
		FLARE_CORE_ASSERT(m_SubMesh.Indicies);
		m_SubMesh.Indicies->SetData(data, count);
	}

	void Mesh::SetVertices(const glm::vec3* data, size_t count)
	{
		FLARE_CORE_ASSERT(m_SubMesh.Vertices);
		m_SubMesh.Vertices->SetData(data, count * sizeof(glm::vec3));
	}

	void Mesh::SetNormals(const glm::vec3* normals, size_t count)
	{
		FLARE_CORE_ASSERT(m_SubMesh.Normals);
		m_SubMesh.Normals->SetData(normals, count * sizeof(glm::vec3));
	}

	void Mesh::SetUVs(const glm::vec2* uvs, size_t count)
	{
		FLARE_CORE_ASSERT(m_SubMesh.UVs);
		m_SubMesh.Normals->SetData(uvs, count * sizeof(glm::vec2));
	}

	void Mesh::SetInstanceBuffer(const Ref<VertexBuffer>& instanceBuffer)
	{
		m_SubMesh.InstanceBuffer = instanceBuffer;
		m_SubMesh.MeshVertexArray->AddInstanceBuffer(instanceBuffer);
	}

	void Mesh::InitializeBuffers()
	{
		m_SubMesh.Vertices->SetLayout({
			{ "i_Position", ShaderDataType::Float3 },
		});

		m_SubMesh.Normals->SetLayout({
			{ "i_Normal", ShaderDataType::Float3 },
		});

		m_SubMesh.MeshVertexArray->AddVertexBuffer(m_SubMesh.Vertices);
		m_SubMesh.MeshVertexArray->AddVertexBuffer(m_SubMesh.Normals);

		if (m_SubMesh.UVs)
		{
			m_SubMesh.UVs->SetLayout({
				{ "i_UV", ShaderDataType::Float2 },
			});
			m_SubMesh.MeshVertexArray->AddVertexBuffer(m_SubMesh.UVs);
		}

		m_SubMesh.MeshVertexArray->SetIndexBuffer(m_SubMesh.Indicies);

		m_SubMesh.MeshVertexArray->Unbind();
	}
}
