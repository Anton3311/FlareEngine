#include "Renderer2D.h"

#include "FlareCore/Core.h"

#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/Renderer.h"

#include <algorithm>

namespace Flare
{
	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 UV;
		float TextuteIndex;
		int32_t EntityIndex;
	};

	constexpr size_t MaxTexturesCount = 32;

	struct Renderer2DData
	{
		Renderer2DStats Stats;

		size_t QuadIndex = 0;
		size_t MaxQuadCount = 0;

		std::vector<QuadVertex> Vertices;

		Ref<VertexArray> VertexArray = nullptr;
		Ref<VertexBuffer> VertexBuffer = nullptr;
		Ref<IndexBuffer> IndexBuffer = nullptr;

		Ref<Texture> WhiteTexture = nullptr;
		Ref<Texture> Textures[MaxTexturesCount];
		uint32_t TextureIndex = 0;

		Ref<Shader> CurrentShader = nullptr;

		glm::vec3 QuadVertices[4];
		glm::vec2 QuadUV[4];

		RenderData* FrameData = nullptr;
	};

	Renderer2DData s_Renderer2DData;

	void Renderer2D::Initialize(size_t maxQuads)
	{
		s_Renderer2DData.QuadIndex = 0;
		s_Renderer2DData.MaxQuadCount = maxQuads;
		s_Renderer2DData.Vertices.resize(maxQuads * 4);
		s_Renderer2DData.CurrentShader = nullptr;

		std::vector<uint32_t> indices(maxQuads * 6);

		for (size_t quadIndex = 0; quadIndex < maxQuads; quadIndex++)
		{
			indices[quadIndex * 6 + 0] = (uint32_t)(quadIndex * 4 + 0);
			indices[quadIndex * 6 + 1] = (uint32_t)(quadIndex * 4 + 1);
			indices[quadIndex * 6 + 2] = (uint32_t)(quadIndex * 4 + 2);
			indices[quadIndex * 6 + 3] = (uint32_t)(quadIndex * 4 + 0);
			indices[quadIndex * 6 + 4] = (uint32_t)(quadIndex * 4 + 2);
			indices[quadIndex * 6 + 5] = (uint32_t)(quadIndex * 4 + 3);
		}

		s_Renderer2DData.VertexArray = VertexArray::Create();
		s_Renderer2DData.VertexBuffer = VertexBuffer::Create(maxQuads * 4 * sizeof(QuadVertex));
		s_Renderer2DData.IndexBuffer = IndexBuffer::Create(maxQuads * 6, indices.data());

		s_Renderer2DData.VertexBuffer->SetLayout({
			BufferLayoutElement("i_Position", ShaderDataType::Float3),
			BufferLayoutElement("i_Color", ShaderDataType::Float4),
			BufferLayoutElement("i_UV", ShaderDataType::Float2),
			BufferLayoutElement("i_TextureIndex", ShaderDataType::Float),
			BufferLayoutElement("i_EntityIndex", ShaderDataType::Int),
		});

		s_Renderer2DData.VertexArray->SetIndexBuffer(s_Renderer2DData.IndexBuffer);
		s_Renderer2DData.VertexArray->AddVertexBuffer(s_Renderer2DData.VertexBuffer);

		s_Renderer2DData.VertexArray->Unbind();

		s_Renderer2DData.QuadVertices[0] = glm::vec3(-0.5f, -0.5f, 0.0f);
		s_Renderer2DData.QuadVertices[1] = glm::vec3(-0.5f, 0.5f, 0.0f);
		s_Renderer2DData.QuadVertices[2] = glm::vec3(0.5f, 0.5f, 0.0f);
		s_Renderer2DData.QuadVertices[3] = glm::vec3(0.5f, -0.5f, 0.0f);

		s_Renderer2DData.QuadUV[0] = glm::vec2(0.0f, 0.0f);
		s_Renderer2DData.QuadUV[1] = glm::vec2(0.0f, 1.0f);
		s_Renderer2DData.QuadUV[2] = glm::vec2(1.0f, 1.0f);
		s_Renderer2DData.QuadUV[3] = glm::vec2(1.0f, 0.0f);

		{
			uint32_t whiteTextureData = 0xffffffff;
			s_Renderer2DData.WhiteTexture = Texture::Create(1, 1, &whiteTextureData, TextureFormat::RGBA8);
		}

		s_Renderer2DData.Textures[0] = s_Renderer2DData.WhiteTexture;
		s_Renderer2DData.TextureIndex = 1;

		s_Renderer2DData.Stats.DrawCalls = 0;
		s_Renderer2DData.Stats.QuadsCount = 0;
	}

	void Renderer2D::Shutdown()
	{
	}

	void Renderer2D::Begin(const Ref<Shader>& shader)
	{
		FLARE_CORE_ASSERT(shader);
		if (s_Renderer2DData.QuadIndex > 0)
			Flush();

		s_Renderer2DData.FrameData = &Renderer::GetCurrentViewport().FrameData;
		s_Renderer2DData.CurrentShader = shader;
	}

	void Renderer2D::Flush()
	{
		FLARE_CORE_ASSERT(s_Renderer2DData.CurrentShader, "Shader was not provided");
		s_Renderer2DData.VertexBuffer->SetData(s_Renderer2DData.Vertices.data(), sizeof(QuadVertex) * s_Renderer2DData.QuadIndex * 4);

		int32_t slots[MaxTexturesCount];
		for (uint32_t i = 0; i < MaxTexturesCount; i++)
			slots[i] = (int32_t)i;

		for (uint32_t i = 0; i < s_Renderer2DData.TextureIndex; i++)
			s_Renderer2DData.Textures[i]->Bind(i);

		s_Renderer2DData.CurrentShader->Bind();
		s_Renderer2DData.CurrentShader->SetMatrix4("u_Projection", s_Renderer2DData.FrameData->Camera.ViewProjection);
		s_Renderer2DData.CurrentShader->SetIntArray("u_Textures", slots, MaxTexturesCount);
		
		RenderCommand::DrawIndexed(s_Renderer2DData.VertexArray, s_Renderer2DData.QuadIndex * 6);

		s_Renderer2DData.QuadIndex = 0;
		s_Renderer2DData.TextureIndex = 1;

		s_Renderer2DData.Stats.DrawCalls++;
	}
	
	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::vec2 halfSize = size / 2.0f;

		if (s_Renderer2DData.QuadIndex >= s_Renderer2DData.MaxQuadCount)
			Flush();

		size_t vertexIndex = s_Renderer2DData.QuadIndex * 4;
		for (uint32_t i = 0; i < 4; i++)
		{
			QuadVertex& vertex = s_Renderer2DData.Vertices[vertexIndex + i];
			vertex.Position = s_Renderer2DData.QuadVertices[i] * glm::vec3(size, 0.0f) + position;
			vertex.Color = color;
			vertex.UV = s_Renderer2DData.QuadUV[i];
			vertex.TextuteIndex = 0; // White texture
			vertex.EntityIndex = INT32_MAX;
		}

		s_Renderer2DData.QuadIndex++;
		s_Renderer2DData.Stats.QuadsCount++;
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& tint, const Ref<Texture>& texture,
		glm::vec2 tiling, int32_t entityIndex, SpriteRenderFlags flags)
	{
		if (s_Renderer2DData.QuadIndex >= s_Renderer2DData.MaxQuadCount)
			Flush();

		if (s_Renderer2DData.TextureIndex == MaxTexturesCount)
			Flush();

		uint32_t textureIndex = 0;
		size_t vertexIndex = s_Renderer2DData.QuadIndex * 4;

		if (texture != nullptr)
		{
			for (textureIndex = 0; textureIndex < s_Renderer2DData.TextureIndex; textureIndex++)
			{
				if (s_Renderer2DData.Textures[textureIndex].get() == texture.get())
					break;
			}

			if (textureIndex >= s_Renderer2DData.TextureIndex)
				s_Renderer2DData.Textures[s_Renderer2DData.TextureIndex++] = texture;
		}

		for (uint32_t i = 0; i < 4; i++)
		{
			QuadVertex& vertex = s_Renderer2DData.Vertices[vertexIndex + i];
			vertex.Position = transform * glm::vec4(s_Renderer2DData.QuadVertices[i], 1.0f);
			vertex.Color = tint;
			vertex.UV = s_Renderer2DData.QuadUV[i] * tiling;
			vertex.TextuteIndex = (float)textureIndex;
			vertex.EntityIndex = entityIndex;
		}

		if (HAS_BIT(flags, SpriteRenderFlags::FlipX))
		{
			for (uint32_t i = 0; i < 4; i++)
				s_Renderer2DData.Vertices[vertexIndex + i].UV.x = 1.0f - s_Renderer2DData.Vertices[vertexIndex + i].UV.x;
		}

		if (HAS_BIT(flags, SpriteRenderFlags::FlipY))
		{
			for (uint32_t i = 0; i < 4; i++)
				s_Renderer2DData.Vertices[vertexIndex + i].UV.y = 1.0f - s_Renderer2DData.Vertices[vertexIndex + i].UV.y;
		}

		s_Renderer2DData.QuadIndex++;
		s_Renderer2DData.Stats.QuadsCount++;
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture>& texture, glm::vec4 tint, glm::vec2 tiling)
	{
		DrawQuad(position, size, texture, tint, tiling, s_Renderer2DData.QuadUV);
	}

	void Renderer2D::DrawSprite(const Sprite& sprite, const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::vec2 uv[4] =
		{
			sprite.GetUVMin(),
			glm::vec2(sprite.GetUVMin().x, sprite.GetUVMax().y),
			sprite.GetUVMax(),
			glm::vec2(sprite.GetUVMax().x, sprite.GetUVMin().y),
		};

		DrawQuad(position, size, sprite.GetAtlas(), color, glm::vec2(1.0f), uv);
	}
	
	void Renderer2D::End()
	{
		if (s_Renderer2DData.QuadIndex > 0)
			Flush();
	}

	void Renderer2D::ResetStats()
	{
		s_Renderer2DData.Stats.DrawCalls = 0;
		s_Renderer2DData.Stats.QuadsCount = 0;
	}

	const Renderer2DStats& Renderer2D::GetStats()
	{
		return s_Renderer2DData.Stats;
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture>& texture, const glm::vec4& tint, const glm::vec2& tiling, const glm::vec2* uv)
	{
		if (s_Renderer2DData.QuadIndex >= s_Renderer2DData.MaxQuadCount)
			Flush();

		if (s_Renderer2DData.TextureIndex == MaxTexturesCount)
			Flush();

		uint32_t textureIndex = 0;
		size_t vertexIndex = s_Renderer2DData.QuadIndex * 4;

		for (textureIndex = 0; textureIndex < s_Renderer2DData.TextureIndex; textureIndex++)
		{
			if (s_Renderer2DData.Textures[textureIndex].get() == texture.get())
				break;
		}

		if (textureIndex >= s_Renderer2DData.TextureIndex)
			s_Renderer2DData.Textures[s_Renderer2DData.TextureIndex++] = texture;

		for (uint32_t i = 0; i < 4; i++)
		{
			QuadVertex& vertex = s_Renderer2DData.Vertices[vertexIndex + i];
			vertex.Position = s_Renderer2DData.QuadVertices[i] * glm::vec3(size, 0.0f) + position;
			vertex.Color = tint;
			vertex.UV = uv[i] * tiling;
			vertex.TextuteIndex = (float)textureIndex;
			vertex.EntityIndex = INT32_MAX;
		}

		s_Renderer2DData.QuadIndex++;
		s_Renderer2DData.Stats.QuadsCount++;
	}
}
