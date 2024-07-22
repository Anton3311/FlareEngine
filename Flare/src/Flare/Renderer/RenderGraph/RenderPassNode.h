#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"

namespace Flare
{
	class FrameBuffer;
	struct RenderPassNode
	{
		RenderGraphPassSpecifications Specifications;
		Ref<RenderGraphPass> Pass = nullptr;
		Ref<FrameBuffer> RenderTarget = nullptr;
		LayoutTransitionsRange Transitions;
	};
}
