#include "Renderer2D.h"

#include "FlareCore/Core.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Math/Math.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Pipeline.h"

#include "Flare/Project/Project.h"

#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"

#include <algorithm>
#include <cctype>

namespace Flare
{
	struct QuadVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 UV;
		int32_t TextuteIndex;
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

		size_t TextQuadIndex = 0;

		Ref<Texture> Textures[MaxTexturesCount];
		uint32_t TextureIndex = 0;

		Ref<Material> DefaultMaterial = nullptr;
		Ref<Material> TextMaterial = nullptr;
		std::optional<uint32_t> FontAtlasPropertyIndex = {};
		Ref<Material> CurrentMaterial = nullptr;

		glm::vec3 QuadVertices[4];
		glm::vec2 QuadUV[4];

		Ref<Font> CurrentFont = nullptr;

		RenderData* FrameData = nullptr;

		Ref<Pipeline> Pipeline = nullptr;
		Ref<VulkanDescriptorSetLayout> DescriptorSetLayout = nullptr;
		Ref<VulkanDescriptorSet> DescriptorSet = nullptr;

		Ref<UniformBuffer> CameraUniformBuffer = nullptr;

		VkDescriptorPool Pool = VK_NULL_HANDLE;
	};

	Renderer2DData s_Renderer2DData;

	static void ReloadShaders();

	void Renderer2D::Initialize(size_t maxQuads)
	{
		s_Renderer2DData.QuadIndex = 0;
		s_Renderer2DData.MaxQuadCount = maxQuads;
		s_Renderer2DData.Vertices.resize(maxQuads * 4);
		s_Renderer2DData.TextVertices.resize(maxQuads * 4);

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
		s_Renderer2DData.IndexBuffer = IndexBuffer::Create(IndexBuffer::IndexFormat::UInt32, MemorySpan::FromVector(indices));

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

		s_Renderer2DData.Textures[0] = Renderer::GetWhiteTexture();
		s_Renderer2DData.TextureIndex = 1;

		s_Renderer2DData.Stats.DrawCalls = 0;
		s_Renderer2DData.Stats.QuadsCount = 0;

		Project::OnProjectOpen.Bind(ReloadShaders);

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			VkDescriptorPoolSize sizes[2];
			sizes[0].descriptorCount = 1;
			sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			sizes[1].descriptorCount = (uint32_t)MaxTexturesCount;
			sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			VkDescriptorPoolCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			info.maxSets = 1;
			info.poolSizeCount = 2;
			info.pPoolSizes = sizes;

			VK_CHECK_RESULT(vkCreateDescriptorPool(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &s_Renderer2DData.Pool));

			VkDescriptorSetLayoutBinding bindings[2] = {};
			bindings[0].binding = 0;
			bindings[0].descriptorCount = 1;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[0].pImmutableSamplers = nullptr;

			bindings[1].binding = 1;
			bindings[1].descriptorCount = (uint32_t)MaxTexturesCount;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[1].pImmutableSamplers = nullptr;

			s_Renderer2DData.CameraUniformBuffer = UniformBuffer::Create(sizeof(RenderView), 0);

			s_Renderer2DData.DescriptorSetLayout = CreateRef<VulkanDescriptorSetLayout>(Span(bindings, 2));
			s_Renderer2DData.DescriptorSet = CreateRef<VulkanDescriptorSet>(s_Renderer2DData.Pool, s_Renderer2DData.DescriptorSetLayout);

			s_Renderer2DData.DescriptorSet->WriteUniformBuffer(s_Renderer2DData.CameraUniformBuffer, 0);
			s_Renderer2DData.DescriptorSet->FlushWrites();
		}
	}

	static void ReloadShaders()
	{
		std::optional<AssetHandle> quadShaderHandle = ShaderLibrary::FindShader("QuadShader");

		if (!quadShaderHandle || !AssetManager::IsAssetHandleValid(quadShaderHandle.value()))
			FLARE_CORE_ERROR("Renderer 2D: Failed to find Quad shader");
		else
		{
			Ref<Shader> quadShader = AssetManager::GetAsset<Shader>(quadShaderHandle.value());
			s_Renderer2DData.DefaultMaterial = CreateRef<Material>(quadShader);
			s_Renderer2DData.CurrentMaterial = s_Renderer2DData.DefaultMaterial;
		}

		std::optional<AssetHandle> textShaderHandle = ShaderLibrary::FindShader("Text");
		if (!textShaderHandle || !AssetManager::IsAssetHandleValid(textShaderHandle.value()))
			FLARE_CORE_ERROR("Renderer 2D: Failed to find Text shader");
		else
		{
			Ref<Shader> textShader = AssetManager::GetAsset<Shader>(textShaderHandle.value());
			s_Renderer2DData.TextMaterial = CreateRef<Material>(textShader);
			s_Renderer2DData.FontAtlasPropertyIndex = textShader->GetPropertyIndex("u_MSDF");
		}
	}

	void Renderer2D::Shutdown()
	{
		s_Renderer2DData.Pipeline = nullptr;
		s_Renderer2DData.CurrentFont = nullptr;
		s_Renderer2DData.CurrentMaterial = nullptr;
		s_Renderer2DData.QuadsMesh = nullptr;
		s_Renderer2DData.QuadsVertexBuffer = nullptr;
		s_Renderer2DData.FrameData = nullptr;
		s_Renderer2DData.IndexBuffer = nullptr;
		s_Renderer2DData.TextMaterial = nullptr;
		s_Renderer2DData.TextMesh = nullptr;
		s_Renderer2DData.TextVertexBuffer = nullptr;

		for (size_t i = 0; i < MaxTexturesCount; i++)
		{
			s_Renderer2DData.Textures[i] = nullptr;
		}

		vkDestroyDescriptorPool(VulkanContext::GetInstance().GetDevice(), s_Renderer2DData.Pool, nullptr);

		s_Renderer2DData = {};
	}

	void Renderer2D::Begin(const Ref<Material>& material)
	{
		if (s_Renderer2DData.QuadIndex > 0)
			FlushAll();

		s_Renderer2DData.FrameData = &Renderer::GetCurrentViewport().FrameData;
		s_Renderer2DData.CurrentMaterial = material;
	}

	void Renderer2D::End()
	{
		FlushAll();

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

	void Renderer2D::SetMaterial(const Ref<Material>& material)
	{
		FlushQuads();
		s_Renderer2DData.CurrentMaterial = material;
	}

	Ref<Material> Renderer2D::GetMaterial()
	{
		return s_Renderer2DData.CurrentMaterial;
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::vec3 vertices[4];
		for (size_t i = 0; i < 4; i++)
			vertices[i] = s_Renderer2DData.QuadVertices[i] * glm::vec3(size, 1.0f) + position;

		DrawQuad(vertices, nullptr, color, glm::vec2(1.0f), s_Renderer2DData.QuadUV, INT32_MAX);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position,
		const glm::vec2& size,
		const glm::vec4& color,
		const Ref<Texture> texture,
		glm::vec2 uvMin, glm::vec2 uvMax)
	{
		glm::vec2 uvs[] =
		{
			uvMin,
			glm::vec2(uvMin.x, uvMax.y),
			uvMax,
			glm::vec2(uvMax.x, uvMin.y)
		};

		glm::vec3 vertices[4];
		for (size_t i = 0; i < 4; i++)
			vertices[i] = s_Renderer2DData.QuadVertices[i] * glm::vec3(size, 1.0f) + position;

		DrawQuad(vertices, texture, color, glm::vec2(1.0f), uvs, INT32_MAX);
	}

	void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& tint, const Ref<Texture>& texture,
		glm::vec2 tiling, int32_t entityIndex, SpriteRenderFlags flags)
	{
		glm::vec3 vertices[4];
		for (size_t i = 0; i < 4; i++)
			vertices[i] = transform * glm::vec4(s_Renderer2DData.QuadVertices[i], 1.0f);

		glm::vec2 uvs[4];
		for (size_t i = 0; i < 4; i++)
			uvs[i] = s_Renderer2DData.QuadUV[i];

		if (HAS_BIT(flags, SpriteRenderFlags::FlipX))
		{
			for (size_t i = 0; i < 4; i++)
				uvs[i].x = 1.0f - uvs[i].x;
		}

		if (HAS_BIT(flags, SpriteRenderFlags::FlipY))
		{
			for (size_t i = 0; i < 4; i++)
				uvs[i].y = 1.0f - uvs[i].y;
		}

		DrawQuad(vertices, texture, tint, tiling, uvs, entityIndex);
	}

	void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture>& texture, glm::vec4 tint, glm::vec2 tiling, int32_t entityIndex)
	{
		glm::vec3 vertices[4];
		for (size_t i = 0; i < 4; i++)
			vertices[i] = s_Renderer2DData.QuadVertices[i] * glm::vec3(size, 1.0f) + position;

		DrawQuad(vertices, texture, tint, tiling, s_Renderer2DData.QuadUV, entityIndex);
	}

	void Renderer2D::DrawSprite(const Sprite& sprite, const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, int32_t entityIndex)
	{
		glm::vec2 uv[4] =
		{
			sprite.UVMin,
			glm::vec2(sprite.UVMin.x, sprite.UVMax.y),
			sprite.UVMax,
			glm::vec2(sprite.UVMax.x, sprite.UVMin.y),
		};

		glm::vec3 vertices[4];
		for (size_t i = 0; i < 4; i++)
			vertices[i] = s_Renderer2DData.QuadVertices[i] * glm::vec3(size, 1.0f) + position;

		DrawQuad(vertices, sprite.GetTexture(), color, glm::vec2(1.0f), uv, entityIndex);
	}

	void Renderer2D::DrawSprite(const Ref<Sprite>& sprite, const glm::mat4& transform, const glm::vec4& color, glm::vec2 tilling, SpriteRenderFlags flags, int32_t entityIndex)
	{
		glm::vec2 uvMin = glm::vec2(0.0f);
		glm::vec2 uvMax = glm::vec2(1.0f);

		if (sprite)
		{
			uvMin = sprite->UVMin;
			uvMax = sprite->UVMax;
		}

		if (HAS_BIT(flags, SpriteRenderFlags::FlipX))
			std::swap(uvMin.x, uvMax.x);
		if (HAS_BIT(flags, SpriteRenderFlags::FlipY))
			std::swap(uvMin.y, uvMax.y);

		glm::vec2 uv[4] =
		{
			uvMin,
			glm::vec2(uvMin.x, uvMax.y),
			uvMax,
			glm::vec2(uvMax.x, uvMin.y),
		};

		for (size_t i = 0; i < 4; i++)
			uv[i] *= tilling;

		glm::vec3 vertices[4];

		for (size_t i = 0; i < 4; i++)
			vertices[i] = transform * glm::vec4(s_Renderer2DData.QuadVertices[i], 1.0f);

		DrawQuad(vertices, sprite == nullptr ? nullptr : sprite->GetTexture(), color, tilling, uv, entityIndex);
	}

	void Renderer2D::DrawQuad(const glm::vec3* vertices, const Ref<Texture>& texture, const glm::vec4& tint, const glm::vec2& tiling, const glm::vec2* uv, int32_t entityIndex)
	{
		if (s_Renderer2DData.QuadIndex >= s_Renderer2DData.MaxQuadCount || s_Renderer2DData.TextureIndex == MaxTexturesCount)
			FlushQuads();

		uint32_t textureIndex = 0;
		size_t vertexIndex = s_Renderer2DData.QuadIndex * 4;

		if (texture != nullptr)
		{
			for (textureIndex = 0; textureIndex < s_Renderer2DData.TextureIndex; textureIndex++)
			{
				if (s_Renderer2DData.Textures[textureIndex].get() == texture.get())
					break;
			}
		}

		if (textureIndex >= s_Renderer2DData.TextureIndex)
			s_Renderer2DData.Textures[s_Renderer2DData.TextureIndex++] = texture;

		for (uint32_t i = 0; i < 4; i++)
		{
			QuadVertex& vertex = s_Renderer2DData.Vertices[vertexIndex + i];
			vertex.Position = vertices[i];
			vertex.Color = tint;
			vertex.UV = uv[i] * tiling;
			vertex.TextuteIndex = textureIndex;
			vertex.EntityIndex = entityIndex;
		}

		s_Renderer2DData.QuadIndex++;
		s_Renderer2DData.Stats.QuadsCount++;
	}

	void Renderer2D::DrawString(std::string_view text, const glm::mat4& transform, const Ref<Font>& font, const glm::vec4& color, int32_t entityIndex)
	{
		FLARE_PROFILE_FUNCTION();

		if (s_Renderer2DData.CurrentFont.get() != font.get())
		{
			FlushText();
			s_Renderer2DData.CurrentFont = font;
		}

		const auto& msdfData = font->GetData();
		const auto& geometry = msdfData.Geometry;
		const auto& metrics = msdfData.Geometry.getMetrics();

		float kerningOffset = 0.0f;
		float lineHeightOffset = 0.0f;

		const Ref<Texture>& fontAtlas = font->GetAtlas();
		glm::vec2 texelSize = glm::vec2(1.0f / fontAtlas->GetWidth(), 1.0f / fontAtlas->GetHeight());
		glm::vec2 position = glm::vec2(0.0f);

		float fontScale = 1.0f / (float)(metrics.ascenderY - metrics.descenderY);
		position.y = -fontScale * (float)metrics.ascenderY;

		const msdf_atlas::GlyphGeometry* errorGlyph = geometry.getGlyph('?');
		const msdf_atlas::GlyphGeometry* spaceGlyph = geometry.getGlyph(' ');
		
		struct Rect
		{
			double Top;
			double Right;
			double Bottom;
			double Left;
		};

		for (size_t charIndex = 0; charIndex < text.size(); charIndex++)
		{
			if (text[charIndex] == 0)
				break;

			const msdf_atlas::GlyphGeometry* glyph = geometry.getGlyph(text[charIndex]);

			if (!glyph)
				glyph = errorGlyph;
			if (!glyph)
				return;

			if (text[charIndex] == '\r')
				continue;
			else if (text[charIndex] == '\t')
				glyph = spaceGlyph;
			else if (text[charIndex] == '\n')
			{
				position.x = 0;
				position.y -= fontScale * (float)metrics.lineHeight + lineHeightOffset;
				continue;
			}
			else if (!std::isspace(text[charIndex]))
			{
				Rect atlasBounds;
				Rect planeBounds;
				glyph->getQuadAtlasBounds(atlasBounds.Left, atlasBounds.Bottom, atlasBounds.Right, atlasBounds.Top);
				glyph->getQuadPlaneBounds(planeBounds.Left, planeBounds.Bottom, planeBounds.Right, planeBounds.Top);

				glm::vec2 min = glm::vec2(planeBounds.Left, planeBounds.Bottom);
				glm::vec2 max = glm::vec2(planeBounds.Right, planeBounds.Top);

				min = min * fontScale + position;
				max = max * fontScale + position;

				atlasBounds.Left *= texelSize.x;
				atlasBounds.Right *= texelSize.x;
				atlasBounds.Top *= texelSize.y;
				atlasBounds.Bottom *= texelSize.y;

				{
					TextVertex& vertex = s_Renderer2DData.TextVertices[s_Renderer2DData.TextQuadIndex * 4 + 0];
					vertex.Position = transform * glm::vec4(min, 0.0f, 1.0f);
					vertex.Color = color;
					vertex.UV = glm::vec2((float)atlasBounds.Left, (float)atlasBounds.Bottom);
					vertex.EntityIndex = entityIndex;
				}

				{
					TextVertex& vertex = s_Renderer2DData.TextVertices[s_Renderer2DData.TextQuadIndex * 4 + 1];
					vertex.Position = transform * glm::vec4(min.x, max.y, 0.0f, 1.0f);
					vertex.Color = color;
					vertex.UV = glm::vec2((float)atlasBounds.Left, (float)atlasBounds.Top);
					vertex.EntityIndex = entityIndex;
				}

				{
					TextVertex& vertex = s_Renderer2DData.TextVertices[s_Renderer2DData.TextQuadIndex * 4 + 2];
					vertex.Position = transform * glm::vec4(max, 0.0f, 1.0f);
					vertex.Color = color;
					vertex.UV = glm::vec2((float)atlasBounds.Right, (float)atlasBounds.Top);
					vertex.EntityIndex = entityIndex;
				}

				{
					TextVertex& vertex = s_Renderer2DData.TextVertices[s_Renderer2DData.TextQuadIndex * 4 + 3];
					vertex.Position = transform * glm::vec4(max.x, min.y, 0.0f, 1.0f);
					vertex.Color = color;
					vertex.UV = glm::vec2((float)atlasBounds.Right, (float)atlasBounds.Bottom);
					vertex.EntityIndex = entityIndex;
				}

				s_Renderer2DData.TextQuadIndex++;
				s_Renderer2DData.Stats.QuadsCount++;

				if (s_Renderer2DData.TextQuadIndex >= s_Renderer2DData.MaxQuadCount)
					FlushText();
			}

			if (charIndex + 1 < text.size())
			{
				double advance = 0.0;

				// TODO: properly handle tabs
				char currentChar = text[charIndex];
				if (currentChar == '\t')
					currentChar = ' ';

				geometry.getAdvance(advance, currentChar, text[charIndex + 1]);

				if (text[charIndex] == '\t')
					advance *= 4.0;

				position.x += fontScale * (float)advance + kerningOffset;
			}
		}
	}

	void Renderer2D::FlushQuads()
	{
		FLARE_PROFILE_FUNCTION();

		if (s_Renderer2DData.QuadIndex == 0)
			return;

		if (s_Renderer2DData.DefaultMaterial == nullptr)
		{
			s_Renderer2DData.QuadIndex = 0;
			s_Renderer2DData.TextureIndex = 0;
			return;
		}

		{
			FLARE_PROFILE_SCOPE("Renderer2D::UpdateQuadsVertexBuffer");
			s_Renderer2DData.QuadsVertexBuffer->SetData(s_Renderer2DData.Vertices.data(), sizeof(QuadVertex) * s_Renderer2DData.QuadIndex * 4);
		}

		for (uint32_t i = s_Renderer2DData.TextureIndex; i < MaxTexturesCount; i++)
			s_Renderer2DData.Textures[i] = Renderer::GetWhiteTexture();

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			if (s_Renderer2DData.Pipeline == nullptr)
			{
				PipelineSpecifications specificaionts{};
				specificaionts.Shader = s_Renderer2DData.DefaultMaterial->GetShader();
				specificaionts.Culling = CullingMode::Back;
				specificaionts.DepthTest = true;
				specificaionts.DepthWrite = true;
				specificaionts.InputLayout = BufferLayout({
					BufferLayoutElement("i_Position", ShaderDataType::Float3),
					BufferLayoutElement("i_Color", ShaderDataType::Float4),
					BufferLayoutElement("i_UV", ShaderDataType::Float2),
					BufferLayoutElement("i_TextureIndex", ShaderDataType::Float),
					BufferLayoutElement("i_EntityIndex", ShaderDataType::Int),
				});

				Ref<VulkanFrameBuffer> renderTarget = As<VulkanFrameBuffer>(Renderer::GetMainViewport().RenderTarget);
				s_Renderer2DData.Pipeline = CreateRef<VulkanPipeline>(specificaionts,
					renderTarget->GetCompatibleRenderPass(),
					Span<Ref<const VulkanDescriptorSetLayout>>((Ref<const VulkanDescriptorSetLayout>)s_Renderer2DData.DescriptorSetLayout));
			}

			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().GetPrimaryCommandBuffer();
			Ref<VulkanFrameBuffer> renderTarget = As<VulkanFrameBuffer>(Renderer::GetCurrentViewport().RenderTarget);

			s_Renderer2DData.CameraUniformBuffer->SetData(&s_Renderer2DData.FrameData->Camera, sizeof(s_Renderer2DData.FrameData->Camera), 0);

			s_Renderer2DData.DescriptorSet->WriteTextures(Span((Ref<const Texture>*)s_Renderer2DData.Textures, MaxTexturesCount), 0, 1);
			s_Renderer2DData.DescriptorSet->FlushWrites();

			commandBuffer->BeginRenderPass(renderTarget->GetCompatibleRenderPass(), renderTarget);
			commandBuffer->BindDescriptorSet(s_Renderer2DData.DescriptorSet, As<const VulkanPipeline>(s_Renderer2DData.Pipeline)->GetLayoutHandle(), 0);
			commandBuffer->BindPipeline(s_Renderer2DData.Pipeline);
			commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f), (glm::vec2)renderTarget->GetSize()));
			commandBuffer->BindVertexBuffers(Span((Ref<const VertexBuffer>)s_Renderer2DData.QuadsVertexBuffer));
			commandBuffer->BindIndexBuffer(s_Renderer2DData.IndexBuffer);
			commandBuffer->DrawIndexed((uint32_t)(s_Renderer2DData.QuadIndex * 6));
			commandBuffer->EndRenderPass();

			s_Renderer2DData.QuadIndex = 0;
			s_Renderer2DData.TextureIndex = 1;

			s_Renderer2DData.Stats.DrawCalls++;

			return;
		}

		int32_t slots[MaxTexturesCount];
		for (uint32_t i = 0; i < MaxTexturesCount; i++)
			slots[i] = (int32_t)i;

		for (uint32_t i = 0; i < s_Renderer2DData.TextureIndex; i++)
			s_Renderer2DData.Textures[i]->Bind(i);

		Ref<Material> material = s_Renderer2DData.CurrentMaterial;
		if (!material)
			material = s_Renderer2DData.DefaultMaterial;

		Ref<Shader> shader = material->GetShader();
		FLARE_CORE_ASSERT(shader);

		std::optional<uint32_t> texturesParameterIndex = shader->GetPropertyIndex("u_Textures");

		if (texturesParameterIndex.has_value())
			material->SetIntArray(texturesParameterIndex.value(), slots, s_Renderer2DData.TextureIndex);

		{
			FLARE_PROFILE_SCOPE("Renderer2D::DrawQuadsMesh");
			Renderer::DrawMesh(s_Renderer2DData.QuadsMesh, material, s_Renderer2DData.QuadIndex * 6);
		}

		s_Renderer2DData.QuadIndex = 0;
		s_Renderer2DData.TextureIndex = 1;

		s_Renderer2DData.Stats.DrawCalls++;
	}

	void Renderer2D::FlushText()
	{
		FLARE_PROFILE_FUNCTION();

		if (s_Renderer2DData.TextQuadIndex == 0)
			return;

		if (s_Renderer2DData.TextMaterial == nullptr || !s_Renderer2DData.FontAtlasPropertyIndex.has_value())
		{
			s_Renderer2DData.TextQuadIndex = 0;
			return;
		}

		if (!s_Renderer2DData.CurrentFont)
			s_Renderer2DData.CurrentFont = Font::GetDefault();

		if (s_Renderer2DData.TextQuadIndex != 0 && s_Renderer2DData.CurrentFont)
		{
			s_Renderer2DData.TextVertexBuffer->SetData(s_Renderer2DData.TextVertices.data(), s_Renderer2DData.TextQuadIndex * sizeof(TextVertex) * 4);

			auto& fontAtlas = s_Renderer2DData.TextMaterial->GetPropertyValue<TexturePropertyValue>(*s_Renderer2DData.FontAtlasPropertyIndex);
			fontAtlas.SetTexture(s_Renderer2DData.CurrentFont->GetAtlas());

			Renderer::DrawMesh(s_Renderer2DData.TextMesh, s_Renderer2DData.TextMaterial, s_Renderer2DData.TextQuadIndex * 6);
		}

		s_Renderer2DData.TextQuadIndex = 0;
	}

	void Renderer2D::FlushAll()
	{
		FLARE_PROFILE_FUNCTION();

		FlushQuads();
		FlushText();
	}
}
