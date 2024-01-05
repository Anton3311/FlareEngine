#include "Mesh.h"

namespace Flare
{
	Mesh::Mesh()
		: Asset(AssetType::Mesh)
	{
	}

	void Mesh::AddSubMesh(const Span<glm::vec3>& vertices,
		IndexBuffer::IndexFormat indexFormat,
		const MemorySpan& indices,
		const Span<glm::vec3>* normals,
		const Span<glm::vec2>* uvs)
	{
		FLARE_CORE_ASSERT(vertices.GetSize() > 0);
		
		if (normals)
			FLARE_CORE_ASSERT(vertices.GetSize() == normals->GetSize());
		if (uvs)
			FLARE_CORE_ASSERT(vertices.GetSize() == uvs->GetSize());

		SubMesh& subMesh = m_SubMeshes.emplace_back();
		subMesh.Bounds = Math::AABB(vertices[0], vertices[0]);

		for (size_t i = 1; i < vertices.GetSize(); i++)
		{
			subMesh.Bounds.Min = glm::min(subMesh.Bounds.Min, vertices[i]);
			subMesh.Bounds.Max = glm::max(subMesh.Bounds.Max, vertices[i]);
		}

		subMesh.Vertices = VertexBuffer::Create(vertices.GetSize() * sizeof(glm::vec3), vertices.GetData());
		subMesh.Indicies = IndexBuffer::Create(indexFormat, indices);

		if (normals)
		{
			subMesh.Normals = VertexBuffer::Create(normals->GetSize() * sizeof(glm::vec3));
			subMesh.Normals->SetData(normals->GetData(), normals->GetSize() * sizeof(glm::vec3));
		}

		if (uvs)
		{
			subMesh.UVs = VertexBuffer::Create(uvs->GetSize() * sizeof(glm::vec2));
			subMesh.UVs->SetData(uvs->GetData(), uvs->GetSize() * sizeof(glm::vec2));
		}

		InitializeLastSubMeshBuffers();
	}

	void Mesh::SetInstanceBuffer(const Ref<VertexBuffer>& instanceBuffer)
	{
		for (SubMesh& subMesh : m_SubMeshes)
		{
			subMesh.VertexArray->AddInstanceBuffer(instanceBuffer);
		}

		m_InstanceBuffer = instanceBuffer;
	}

	void Mesh::InitializeLastSubMeshBuffers()
	{
		FLARE_CORE_ASSERT(m_SubMeshes.size() > 0);

		SubMesh& subMesh = m_SubMeshes.back();
		subMesh.VertexArray = VertexArray::Create();

		subMesh.Vertices->SetLayout({ { "i_Position", ShaderDataType::Float3 } });
		subMesh.VertexArray->AddVertexBuffer(subMesh.Vertices);

		if (subMesh.Normals)
		{
			subMesh.Normals->SetLayout({ { "i_Normal", ShaderDataType::Float3 } });
			subMesh.VertexArray->AddVertexBuffer(subMesh.Normals);
		}

		if (subMesh.UVs)
		{
			subMesh.UVs->SetLayout({ { "i_UV", ShaderDataType::Float2 } });
			subMesh.VertexArray->AddVertexBuffer(subMesh.UVs);
		}

		subMesh.VertexArray->SetIndexBuffer(subMesh.Indicies);
		subMesh.VertexArray->Unbind();
	}
}
