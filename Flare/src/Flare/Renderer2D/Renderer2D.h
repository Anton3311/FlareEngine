#pragma once

#include "Flare.h"

#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Font.h"

#include <vector>

namespace Flare
{
	class VulkanDescriptorSet;
	class VulkanDescriptorSetPool;

	struct Renderer2DStats
	{
		uint32_t QuadsCount = 0;
		uint32_t DrawCalls = 0;

		uint32_t GetTotalVertexCount() const { return QuadsCount * 4; }
	};

	enum class SpriteRenderFlags : uint8_t
	{
		None = 0,
		FlipX = 1,
		FlipY = 2,
	};

	FLARE_IMPL_ENUM_BITFIELD(SpriteRenderFlags);

	struct Renderer2DLimits;
	struct SceneSubmition;

	class Viewport;
	class DescriptorSetLayout;
	class FLARE_API Renderer2D
	{
	public:
		static void Initialize(size_t maxQuads = 10000);
		static void Shutdown();

		static void BeginFrame();
		static void EndFrame();

		static void BeginScene(SceneSubmition& sceneSubmition);
		static void EndScene();

		static void ConfigurePasses(Viewport& viewport);
		
		static void SetMaterial(const Ref<Material>& material);
		static Ref<Material> GetMaterial();

		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color);
		static void DrawQuad(const glm::vec3& position,
			const glm::vec2& size,
			const glm::vec4& color,
			const Ref<Texture> texture,
			glm::vec2 uvMin, glm::vec2 uvMax);

		static void DrawQuad(const glm::mat4& transform,
			const glm::vec4& tint,
			const Ref<Texture>& texture,
			glm::vec2 tiling,
			int32_t entityIndex,
			SpriteRenderFlags flags = SpriteRenderFlags::None);

		static void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
			const Ref<Texture>& texture, 
			glm::vec4 tint = glm::vec4(1), 
			glm::vec2 tilling = glm::vec2(1),
			int32_t entityIndex = INT32_MAX);

		static void DrawSprite(const Sprite& sprite,
			const glm::vec3& position,
			const glm::vec2& size,
			const glm::vec4& color = glm::vec4(1.0f),
			int32_t entityIndex = INT32_MAX);

		static void DrawSprite(const Ref<Sprite>& sprite,
			const glm::mat4& transform,
			const glm::vec4& color = glm::vec4(1.0f),
			glm::vec2 tilling = glm::vec2(1.0f),
			SpriteRenderFlags flags = SpriteRenderFlags::None,
			int32_t entityIndex = INT32_MAX);

		// Text

		static void DrawString(
			std::string_view text,
			const glm::mat4& transform,
			const Ref<Font>& font,
			const glm::vec4& color = glm::vec4(1.0f),
			int32_t entityIndex = INT32_MAX);

		static Ref<const DescriptorSetLayout> GetDescriptorSetLayout();

		static const Renderer2DLimits& GetLimits();

		static void ResetStats();
		static const Renderer2DStats& GetStats();
	private:
		static void DrawQuad(const glm::vec3* vertices, 
			const Ref<Texture>& texture, 
			const glm::vec4& tint,
			const glm::vec2& tiling, 
			const glm::vec2* uv,
			int32_t entityIndex);
	};
}
