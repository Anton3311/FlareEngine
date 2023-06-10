#include "Renderer2D.h"

#include <Flare/Renderer/RenderCommand.h>

namespace Flare
{
	Renderer2DData* Renderer2D::s_Data = nullptr;

	void Renderer2D::Initialize(size_t maxQuads)
	{
		s_Data = new Renderer2DData();
		s_Data->QuadIndex = 0;
		s_Data->MaxQuadCount = maxQuads;
		s_Data->Vertices.resize(maxQuads * 4);
		s_Data->CurrentShader = nullptr;

		std::vector<uint32_t> indices(maxQuads * 6);

		for (size_t quadIndex = 0; quadIndex < maxQuads; quadIndex++)
		{
			indices[quadIndex * 6 + 0] = quadIndex * 4 + 0;
			indices[quadIndex * 6 + 1] = quadIndex * 4 + 1;
			indices[quadIndex * 6 + 2] = quadIndex * 4 + 2;
			indices[quadIndex * 6 + 3] = quadIndex * 4 + 0;
			indices[quadIndex * 6 + 4] = quadIndex * 4 + 2;
			indices[quadIndex * 6 + 5] = quadIndex * 4 + 3;
		}

		s_Data->VertexArray = VertexArray::Create();
		s_Data->VertexBuffer = VertexBuffer::Create(maxQuads * 4 * sizeof(QuadVertex));
		s_Data->IndexBuffer = IndexBuffer::Create(maxQuads * 6, indices.data());

		s_Data->VertexBuffer->SetLayout({
			BufferLayoutElement("i_Position", ShaderDataType::Float2),
			BufferLayoutElement("i_Color", ShaderDataType::Float4),
		});

		s_Data->VertexArray->SetIndexBuffer(s_Data->IndexBuffer);
		s_Data->VertexArray->AddVertexBuffer(s_Data->VertexBuffer);

		s_Data->VertexArray->Unbind();
	}

	void Renderer2D::Shutdown()
	{
		delete s_Data;
	}

	void Renderer2D::Begin(const Ref<Shader>& shader, const glm::mat4& projectionMatrix)
	{
		if (s_Data->QuadIndex > 0 && s_Data->CurrentShader != nullptr)
			Flush();

		s_Data->CameraProjectionMatrix = projectionMatrix;
		s_Data->CurrentShader = shader;
	}
	
	void Renderer2D::Flush()
	{
		s_Data->VertexBuffer->SetData(s_Data->Vertices.data(), sizeof(QuadVertex) * s_Data->QuadIndex * 4);

		s_Data->CurrentShader->Bind();
		s_Data->CurrentShader->SetMatrix4("u_Projection", s_Data->CameraProjectionMatrix);
		RenderCommand::DrawIndexed(s_Data->VertexArray, s_Data->QuadIndex * 6);

		s_Data->QuadIndex = 0;
	}
	
	void Renderer2D::Submit(glm::vec2 position, glm::vec2 size, glm::vec4 color)
	{
		glm::vec2 halfSize = size / 2.0f;

		if (s_Data->QuadIndex >= s_Data->MaxQuadCount)
			Flush();

		size_t vertexIndex = s_Data->QuadIndex * 4;

		{
			QuadVertex& vertex = s_Data->Vertices[vertexIndex];
			vertex.Color = color;
			vertex.Position = position - halfSize;
		}

		{
			QuadVertex& vertex = s_Data->Vertices[vertexIndex + 1];
			vertex.Color = color;
			vertex.Position = position + glm::vec2(-halfSize.x, halfSize.y);
		}

		{
			QuadVertex& vertex = s_Data->Vertices[vertexIndex + 2];
			vertex.Color = color;
			vertex.Position = position + halfSize;
		}

		{
			QuadVertex& vertex = s_Data->Vertices[vertexIndex + 3];
			vertex.Color = color;
			vertex.Position = position + glm::vec2(halfSize.x, -halfSize.y);
		}

		s_Data->QuadIndex++;
	}
	
	void Renderer2D::End()
	{
		if (s_Data->QuadIndex > 0)
			Flush();
	}
}
