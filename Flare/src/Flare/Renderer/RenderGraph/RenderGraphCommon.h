#pragma once

#include "FlareCore/Core.h"

#include "Flare/Renderer/Texture.h"

#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

#include <stdint.h>

namespace Flare
{
	struct LayoutTransition
	{
		RenderGraphTextureId Texture;
		ImageLayout InitialLayout = ImageLayout::Undefined;
		ImageLayout FinalLayout = ImageLayout::Undefined;
	};

	struct ExternalRenderGraphResource
	{
		RenderGraphTextureId Texture;
		ImageLayout InitialLayout = ImageLayout::Undefined;
		ImageLayout FinalLayout = ImageLayout::Undefined;
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
