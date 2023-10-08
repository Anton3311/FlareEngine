#include "Renderer2D.h"

#include "FlareCore/Core.h"

#include "Flare/AssetManager/AssetManager.h"

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

	struct TextVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 UV;
		int32_t EntityIndex;
	};

	constexpr size_t MaxTexturesCount = 32;

	struct Renderer2DData
	{
		Renderer2DStats Stats;

		size_t QuadIndex = 0;
		size_t MaxQuadCount = 0;

		std::vector<QuadVertex> Vertices;

		Ref<VertexArray> QuadsMesh = nullptr;
		Ref<VertexBuffer> QuadsVertexBuffer = nullptr;
		Ref<IndexBuffer> IndexBuffer = nullptr;

		std::vector<TextVertex> TextVertices;
		Ref<VertexArray> TextMesh = nullptr;
		Ref<VertexBuffer> TextVertexBuffer = nullptr;

		size_t TextVertexIndex = 0;

		Ref<Texture> WhiteTexture = nullptr;
		Ref<Texture> Textures[MaxTexturesCount];
		uint32_t TextureIndex = 0;

		Ref<Shader> DefaultShader = nullptr;
		Ref<Shader> TextShader = nullptr;
		Ref<Material> CurrentMaterial = nullptr;

		glm::vec3 QuadVertices[4];
		glm::vec2 QuadUV[4];

		Ref<Font> CurrentFont = nullptr;

		RenderData* FrameData = nullptr;
	};

	Renderer2DData s_Renderer2DData;

	void Renderer2D::Initialize(size_t maxQuads)
	{
		s_Renderer2DData.QuadIndex = 0;
		s_Renderer2DData.MaxQuadCount = maxQuads;
		s_Renderer2DData.Vertices.resize(maxQuads * 4);
		s_Renderer2DData.TextVertices.resize(maxQuads * 4);

		s_Renderer2DData.DefaultShader = Shader::Create("Assets/Shaders/QuadShader.glsl");
		s_Renderer2DData.TextShader = Shader::Create("Assets/Shaders/Text.glsl");

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

		s_Renderer2DData.QuadsMesh = VertexArray::Create();
		s_Renderer2DData.QuadsVertexBuffer = VertexBuffer::Create(maxQuads * 4 * sizeof(QuadVertex));
		s_Renderer2DData.IndexBuffer = IndexBuffer::Create(maxQuads * 6, indices.data());

		s_Renderer2DData.QuadsVertexBuffer->SetLayout({
			BufferLayoutElement("i_Position", ShaderDataType::Float3),
			BufferLayoutElement("i_Color", ShaderDataType::Float4),
			BufferLayoutElement("i_UV", ShaderDataType::Float2),
			BufferLayoutElement("i_TextureIndex", ShaderDataType::Float),
			BufferLayoutElement("i_EntityIndex", ShaderDataType::Int),
		});

		s_Renderer2DData.QuadsMesh->SetIndexBuffer(s_Renderer2DData.IndexBuffer);
		s_Renderer2DData.QuadsMesh->AddVertexBuffer(s_Renderer2DData.QuadsVertexBuffer);

		s_Renderer2DData.QuadsMesh->Unbind();

		// Text
		s_Renderer2DData.TextMesh = VertexArray::Create();
		s_Renderer2DData.TextVertexBuffer = VertexBuffer::Create(maxQuads * 4 * sizeof(TextVertex));
		s_Renderer2DData.TextVertexBuffer->SetLayout({
			BufferLayoutElement("i_Position", ShaderDataType::Float3),
			BufferLayoutElement("i_Color", ShaderDataType::Float4),
			BufferLayoutElement("i_UV", ShaderDataType::Float2),
			BufferLayoutElement("i_EntityIndex", ShaderDataType::Int),
		});

		s_Renderer2DData.TextMesh->SetIndexBuffer(s_Renderer2DData.IndexBuffer);
		s_Renderer2DData.TextMesh->AddVertexBuffer(s_Renderer2DData.TextVertexBuffer);

		s_Renderer2DData.TextMesh->Unbind();


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

	void Renderer2D::Begin(const Ref<Material>& material)
	{
		if (s_Renderer2DData.QuadIndex > 0)
			Flush();

		s_Renderer2DData.FrameData = &Renderer::GetCurrentViewport().FrameData;
		s_Renderer2DData.CurrentMaterial = material;
	}

	void Renderer2D::Flush()
	{
		s_Renderer2DData.QuadsVertexBuffer->SetData(s_Renderer2DData.Vertices.data(), sizeof(QuadVertex) * s_Renderer2DData.QuadIndex * 4);

		int32_t slots[MaxTexturesCount];
		for (uint32_t i = 0; i < MaxTexturesCount; i++)
			slots[i] = (int32_t)i;

		for (uint32_t i = 0; i < s_Renderer2DData.TextureIndex; i++)
			s_Renderer2DData.Textures[i]->Bind(i);

		if (s_Renderer2DData.CurrentMaterial)
		{
			Ref<Shader> shader = AssetManager::GetAsset<Shader>(s_Renderer2DData.CurrentMaterial->GetShaderHandle());
			FLARE_CORE_ASSERT(shader);

			std::optional<uint32_t> texturesParameterIndex = shader->GetParameterIndex("u_Textures");

			if (texturesParameterIndex.has_value())
				s_Renderer2DData.CurrentMaterial->SetIntArray(texturesParameterIndex.value(), slots, s_Renderer2DData.TextureIndex);

			Renderer::DrawMesh(s_Renderer2DData.QuadsMesh, s_Renderer2DData.CurrentMaterial, s_Renderer2DData.QuadIndex * 6);
		}
		else
		{
			s_Renderer2DData.DefaultShader->Bind();
			s_Renderer2DData.DefaultShader->SetIntArray("u_Textures", slots, s_Renderer2DData.TextureIndex);

			RenderCommand::DrawIndexed(s_Renderer2DData.QuadsMesh, s_Renderer2DData.QuadIndex * 6);
		}

		if (s_Renderer2DData.TextVertexIndex != 0)
		{
			s_Renderer2DData.TextVertexBuffer->SetData(s_Renderer2DData.TextVertices.data(), s_Renderer2DData.TextVertexIndex * sizeof(TextVertex) * 4);

			s_Renderer2DData.CurrentFont->GetAtlas()->Bind();
			s_Renderer2DData.TextShader->Bind();
			
			RenderCommand::DrawIndexed(s_Renderer2DData.TextMesh, s_Renderer2DData.TextVertexIndex * 6);
		}

		s_Renderer2DData.QuadIndex = 0;
		s_Renderer2DData.TextureIndex = 1;

		s_Renderer2DData.TextVertexIndex = 0;

		s_Renderer2DData.Stats.DrawCalls++;
	}

	void Renderer2D::SetMaterial(const Ref<Material>& material)
	{
		Flush();
		s_Renderer2DData.CurrentMaterial = material;
	}

	Ref<Material> Renderer2D::GetMaterial()
	{
		return s_Renderer2DData.CurrentMaterial;
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

		s_Renderer2DData.CurrentFont = nullptr;
		s_Renderer2DData.CurrentMaterial = nullptr;
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

	void Renderer2D::DrawString(std::string_view text, const glm::mat4& transform, const Ref<Font>& font, const glm::vec4& color, int32_t entityIndex)
	{
		s_Renderer2DData.CurrentFont = font;

		const auto& msdfData = font->GetData();
		const auto& geometry = msdfData.Geometry;
		const auto& metrics = msdfData.Geometry.getMetrics();

		const Ref<Texture>& fontAtlas = font->GetAtlas();

		glm::vec2 position = glm::vec2(0.0f);

		float fontScale = 1.0f / (float)(metrics.ascenderY - metrics.descenderY);
		position.y = -fontScale * (float)metrics.ascenderY;

		for (size_t charIndex = 0; charIndex < text.size(); charIndex++)
		{
			if (text[charIndex] == 0)
				break;

			auto glyph = geometry.getGlyph(text[charIndex]);

			if (!glyph)
				glyph = geometry.getGlyph('?');
			if (!glyph)
				return;
			
			struct Rect
			{
				double Top;
				double Right;
				double Bottom;
				double Left;
			};

			Rect atlasBounds;
			Rect planeBounds;
			glyph->getQuadAtlasBounds(atlasBounds.Left, atlasBounds.Bottom, atlasBounds.Right, atlasBounds.Top);
			glyph->getQuadPlaneBounds(planeBounds.Left, planeBounds.Bottom, planeBounds.Right, planeBounds.Top);

			glm::vec2 min = glm::vec2(planeBounds.Left, planeBounds.Bottom);
			glm::vec2 max = glm::vec2(planeBounds.Right, planeBounds.Top);

			min *= fontScale;
			max *= fontScale;

			min += position;
			max += position;

			glm::vec2 texelSize = glm::vec2(1.0f / fontAtlas->GetWidth(), 1.0f / fontAtlas->GetHeight());

			atlasBounds.Left *= texelSize.x;
			atlasBounds.Right *= texelSize.x;
			atlasBounds.Top *= texelSize.y;
			atlasBounds.Bottom *= texelSize.y;

			{
				TextVertex& vertex = s_Renderer2DData.TextVertices[s_Renderer2DData.TextVertexIndex * 4 + 0];
				vertex.Position = transform * glm::vec4(min, 0.0f, 1.0f);
				vertex.Color = color;
				vertex.UV = glm::vec2((float)atlasBounds.Left, (float)atlasBounds.Bottom);
				vertex.EntityIndex = entityIndex;
			}

			{
				TextVertex& vertex = s_Renderer2DData.TextVertices[s_Renderer2DData.TextVertexIndex * 4 + 1];
				vertex.Position = transform * glm::vec4(min.x, max.y, 0.0f, 1.0f);
				vertex.Color = color;
				vertex.UV = glm::vec2((float)atlasBounds.Left, (float)atlasBounds.Top);
				vertex.EntityIndex = entityIndex;
			}

			{
				TextVertex& vertex = s_Renderer2DData.TextVertices[s_Renderer2DData.TextVertexIndex * 4 + 2];
				vertex.Position = transform * glm::vec4(max, 0.0f, 1.0f);
				vertex.Color = color;
				vertex.UV = glm::vec2((float)atlasBounds.Right, (float)atlasBounds.Top);
				vertex.EntityIndex = entityIndex;
			}

			{
				TextVertex& vertex = s_Renderer2DData.TextVertices[s_Renderer2DData.TextVertexIndex * 4 + 3];
				vertex.Position = transform * glm::vec4(max.x, min.y, 0.0f, 1.0f);
				vertex.Color = color;
				vertex.UV = glm::vec2((float)atlasBounds.Right, (float)atlasBounds.Bottom);
				vertex.EntityIndex = entityIndex;
			}

			s_Renderer2DData.TextVertexIndex++;
			s_Renderer2DData.Stats.QuadsCount++;

			double advance = 0.0;
			if (charIndex + 1 == text.size())
				geometry.getAdvance(advance, text[charIndex], 0);
			else
				geometry.getAdvance(advance, text[charIndex], text[charIndex + 1]);

			float kerningOffset = 0.0f;
			position.x += fontScale * (float)advance + kerningOffset;
		}
	}
}
