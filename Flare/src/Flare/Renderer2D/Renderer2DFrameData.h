#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include <glm/glm.hpp>
#include <vector>

namespace Flare
{
	class Font;
	class Texture;
	class Material;
	class DescriptorSet;
	class DescriptorSetPool;

	struct Renderer2DLimits
	{
		static constexpr uint32_t MaxTexturesCount = 32;

		uint32_t MaxQuadCount = 0;
	};

	struct QuadVertex
	{
		glm::vec3 Position = glm::vec3(0.0f);
		glm::vec4 Color = glm::vec4(1.0f);
		glm::vec2 UV = glm::vec2(0.0f);
		int32_t TextureIndex = 0;
		int32_t EntityIndex = 0;
	};

	struct QuadsBatch
	{
		inline uint32_t GetEnd() const { return Start + Count; }

		uint32_t GetTextureIndex(const Ref<Texture>& texture)
		{
			FLARE_CORE_ASSERT(TexturesCount < Renderer2DLimits::MaxTexturesCount);
			FLARE_CORE_ASSERT(texture);

			for (uint32_t i = 0; i < TexturesCount; i++)
			{
				if (texture.get() == Textures[i].get())
				{
					return i;
				}
			}

			TexturesCount++;

			Textures[TexturesCount - 1] = texture;
			return TexturesCount - 1;
		}

		Ref<Material> Material = nullptr;
		Ref<Texture> Textures[Renderer2DLimits::MaxTexturesCount] = { nullptr };
		uint32_t TexturesCount = 0;
		uint32_t Start = 0;
		uint32_t Count = 0;
	};

	struct TextVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
		glm::vec2 UV;
		int32_t EntityIndex;
	};

	struct TextBatch
	{
		inline size_t GetEnd() const { return Start + Count; }

		Ref<const Font> Font = nullptr;
		uint32_t Start = 0;
		uint32_t Count = 0;
	};

	struct FLARE_API Renderer2DFrameData
	{
		void Reset();

		// Quads
		size_t QuadCount = 0;
		std::vector<QuadVertex> QuadVertices;
		std::vector<QuadsBatch> QuadBatches;

		Ref<DescriptorSetPool> QuadDescriptorSetsPool = nullptr;
		std::vector<Ref<DescriptorSet>> UsedQuadDescriptorSets;

		// Text
		size_t TextQuadCount = 0;
		std::vector<TextVertex> TextVertices;
		std::vector<TextBatch> TextBatches;

		Ref<DescriptorSetPool> TextDescriptorSetsPool = nullptr;
		std::vector<Ref<DescriptorSet>> UsedTextDescriptorSets;
	};
}
