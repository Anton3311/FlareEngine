#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/Texture.h"

#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

#include <stdint.h>

namespace Flare
{
	enum class AttachmentClearValueType : uint8_t
	{
		Color = 0,
		Depth = 1,
	};

	struct AttachmentClearValue
	{
		AttachmentClearValue() = default;
		AttachmentClearValue(const glm::vec4& clearColor)
			: Type(AttachmentClearValueType::Color), Color(clearColor) {}
		AttachmentClearValue(float clearDepth)
			: Type(AttachmentClearValueType::Depth), Depth(clearDepth) {}

		AttachmentClearValueType Type = AttachmentClearValueType::Color;
		union
		{
			glm::vec4 Color = glm::vec4(0.0f);
			float Depth;
		};
	};

	struct LayoutTransition
	{
		RenderGraphTextureId Texture;
		TextureSubresource Subresource;
		ImageLayout InitialLayout = ImageLayout::Undefined;
		ImageLayout FinalLayout = ImageLayout::Undefined;
	};

	struct ExternalRenderGraphResource
	{
		RenderGraphTextureId Texture;
		ImageLayout InitialLayout = ImageLayout::Undefined;
		ImageLayout FinalLayout = ImageLayout::Undefined;
		std::optional<AttachmentClearValue> ClearValue;
	};

	struct LayoutTransitionsRange
	{
		LayoutTransitionsRange() = default;

		LayoutTransitionsRange(uint32_t startAndEnd)
			: Start(startAndEnd), End(startAndEnd) {}

		LayoutTransitionsRange(uint32_t start, uint32_t end)
			: Start(start), End(end) {}

		uint32_t Start = UINT32_MAX;
		uint32_t End = UINT32_MAX;
	};

	struct FLARE_API CompiledRenderGraph
	{
		void Reset();

		std::vector<LayoutTransition> LayoutTransitions;
		LayoutTransitionsRange ExternalResourceFinalTransitions;
	};
}
