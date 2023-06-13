#include "Renderer2D.h"

#include "Flare/Renderer/RenderCommand.h"

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
			BufferLayoutElement("i_Position", ShaderDataType::Float3),
			BufferLayoutElement("i_Color", ShaderDataType::Float4),
			BufferLayoutElement("i_UV", ShaderDataType::Float2),
			BufferLayoutElement("i_TextureIndex", ShaderDataType::Float),
			BufferLayoutElement("i_TextureTilling", ShaderDataType::Float2),
		});

		s_Data->VertexArray->SetIndexBuffer(s_Data->IndexBuffer);
		s_Data->VertexArray->AddVertexBuffer(s_Data->VertexBuffer);

		s_Data->VertexArray->Unbind();

		s_Data->QuadVertices[0] = glm::vec3(-0.5f, -0.5f, 0.0f);
		s_Data->QuadVertices[1] = glm::vec3(-0.5f, 0.5f, 0.0f);
		s_Data->QuadVertices[2] = glm::vec3(0.5f, 0.5f, 0.0f);
		s_Data->QuadVertices[3] = glm::vec3(0.5f, -0.5f, 0.0f);

		s_Data->QuadUV[0] = glm::vec2(0.0f, 0.0f);
		s_Data->QuadUV[1] = glm::vec2(0.0f, 1.0f);
		s_Data->QuadUV[2] = glm::vec2(1.0f, 1.0f);
		s_Data->QuadUV[3] = glm::vec2(1.0f, 0.0f);

		{
			uint32_t whiteTextureData = 0xffffffff;
			s_Data->WhiteTexture = Texture::Create(1, 1, &whiteTextureData, TextureFormat::RGBA8);
		}

		s_Data->Textures[0] = s_Data->WhiteTexture;
		s_Data->TextureIndex = 1;
	}

	void Renderer2D::Shutdown()
	{
		delete s_Data;
		s_Data = nullptr;
	}

	void Renderer2D::Begin(const Ref<Shader>& shader, const glm::mat4& projectionMatrix)
	{
		FLARE_CORE_ASSERT(shader);
		if (s_Data->QuadIndex > 0)
			Flush();

		s_Data->CameraProjectionMatrix = projectionMatrix;
		s_Data->CurrentShader = shader;
	}
	
	void Renderer2D::Flush()
	{
		FLARE_CORE_ASSERT(s_Data->CurrentShader, "Shader was not provided");
		s_Data->VertexBuffer->SetData(s_Data->Vertices.data(), sizeof(QuadVertex) * s_Data->QuadIndex * 4);

		int32_t slots[MaxTexturesCount];
		for (size_t i = 0; i < MaxTexturesCount; i++)
			slots[i] = i;

		for (int32_t i = 0; i < s_Data->TextureIndex; i++)
			s_Data->Textures[i]->Bind(i);

		s_Data->CurrentShader->Bind();
		s_Data->CurrentShader->SetMatrix4("u_Projection", s_Data->CameraProjectionMatrix);
		s_Data->CurrentShader->SetIntArray("u_Textures", slots, MaxTexturesCount);
		
		RenderCommand::DrawIndexed(s_Data->VertexArray, s_Data->QuadIndex * 6);

		s_Data->QuadIndex = 0;
		s_Data->TextureIndex = 1;
	}
	
	void Renderer2D::DrawQuad(glm::vec3 position, glm::vec2 size, glm::vec4 color)
	{
		glm::vec2 halfSize = size / 2.0f;

		if (s_Data->QuadIndex >= s_Data->MaxQuadCount)
			Flush();

		size_t vertexIndex = s_Data->QuadIndex * 4;
		for (uint32_t i = 0; i < 4; i++)
		{
			QuadVertex& vertex = s_Data->Vertices[vertexIndex + i];
			vertex.Position = s_Data->QuadVertices[i] * glm::vec3(size, 0.0f) + position;
			vertex.Color = color;
			vertex.UV = s_Data->QuadUV[i];
			vertex.TextuteIndex = 0; // White texture
		}

		s_Data->QuadIndex++;
	}

	void Renderer2D::DrawQuad(glm::vec3 position, glm::vec2 size, const Ref<Texture>& texture, glm::vec4 tint, glm::vec2 tiling)
	{

		if (s_Data->QuadIndex >= s_Data->MaxQuadCount)
			Flush();

		if (s_Data->TextureIndex == MaxTexturesCount)
			Flush();

		int32_t textureIndex = 0;
		size_t vertexIndex = s_Data->QuadIndex * 4;

		for (textureIndex = 0; textureIndex < s_Data->TextureIndex; textureIndex++)
		{
			if (s_Data->Textures[textureIndex].get() == texture.get())
				break;
		}

		if (textureIndex >= s_Data->TextureIndex)
			s_Data->Textures[s_Data->TextureIndex++] = texture;

		glm::vec2 halfSize = size / 2.0f;
		for (uint32_t i = 0; i < 4; i++)
		{
			QuadVertex& vertex = s_Data->Vertices[vertexIndex + i];
			vertex.Position = s_Data->QuadVertices[i] * glm::vec3(size, 0.0f) + position;
			vertex.Color = tint;
			vertex.UV = s_Data->QuadUV[i];
			vertex.TextureTiling = tiling;
			vertex.TextuteIndex = textureIndex;
		}

		s_Data->QuadIndex++;
	}
	
	void Renderer2D::End()
	{
		if (s_Data->QuadIndex > 0)
			Flush();
	}
}
