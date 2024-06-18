#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/FrameBuffer.h"

namespace Flare
{
	struct LayoutTransition
	{
		Ref<Texture> TextureHandle = nullptr;
		ImageLayout InitialLayout = ImageLayout::Undefined;
		ImageLayout FinalLayout = ImageLayout::Undefined;
	};

	struct RenderPassNode
	{
		RenderGraphPassSpecifications Specifications;
		Ref<RenderGraphPass> Pass = nullptr;
		Ref<FrameBuffer> RenderTarget = nullptr;
		std::vector<LayoutTransition> Transitions;
	};
}
