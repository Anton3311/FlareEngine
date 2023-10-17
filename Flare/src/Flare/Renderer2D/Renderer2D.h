#pragma once

#include "Flare.h"

#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Font.h"

#include <vector>

namespace Flare
{
	struct Renderer2DStats
	{
		uint32_t QuadsCount;
		uint32_t DrawCalls;

		uint32_t GetTotalVertexCount() const { return QuadsCount * 4; }
	};

	enum class SpriteRenderFlags : uint8_t
	{
		None = 0,
		FlipX = 1,
		FlipY = 2,
	};

	constexpr SpriteRenderFlags operator|(SpriteRenderFlags a, SpriteRenderFlags b)
	{
		return (SpriteRenderFlags)((uint8_t)a | (uint8_t)b);
	}

	constexpr SpriteRenderFlags operator~(SpriteRenderFlags a)
	{
		return (SpriteRenderFlags)(~(uint8_t)a);
	}

	constexpr void operator|=(SpriteRenderFlags& a, SpriteRenderFlags b)
	{
		a = (SpriteRenderFlags)((uint8_t)a | (uint8_t)b);
	}

	constexpr SpriteRenderFlags operator&(SpriteRenderFlags a, SpriteRenderFlags b)
	{
		return (SpriteRenderFlags)((uint8_t)a & (uint8_t)b);
	}

	constexpr bool operator!=(SpriteRenderFlags a, int b)
	{
		return (int)a != b;
	}

	class FLARE_API Renderer2D
	{
	public:
		static void Initialize(size_t maxQuads = 10000);
		static void Shutdown();

		static void Begin(const Ref<Material>& material = nullptr);
		static void End();
		
		static void SetMaterial(const Ref<Material>& material);
		static Ref<Material> GetMaterial();

		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);

		static void DrawQuad(const glm::mat4& transform,
			const glm::vec4& tint,
			const Ref<Texture>& texture,
			glm::vec2 tiling,
			int32_t entityIndex,
			SpriteRenderFlags flags = SpriteRenderFlags::None);

		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
			const Ref<Texture>& texture, 
			glm::vec4 tint = glm::vec4(1), 
			glm::vec2 tilling = glm::vec2(1));

		static void DrawSprite(const Sprite& sprite, const glm::vec3& position, const glm::vec2& size, const glm::vec4& color = glm::vec4(1.0f));

		// Text

		static void DrawString(
			std::string_view text,
			const glm::mat4& transform,
			const Ref<Font>& font,
			const glm::vec4& color = glm::vec4(1.0f),
			int32_t entityIndex = INT32_MAX);

		static void ResetStats();
		static const Renderer2DStats& GetStats();
	private:
		static void FlushQuads();
		static void FlushText();
		static void FlushAll();
	private:
		static void DrawQuad(const glm::vec3& position, 
			const glm::vec2& size, 
			const Ref<Texture>& texture, 
			const glm::vec4& tint,
			const glm::vec2& tiling, 
			const glm::vec2* uv);
	};
}
