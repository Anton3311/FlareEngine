#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"

namespace Flare
{
	struct RenderPassNode
	{
		static constexpr uint32_t INVALID_TARGET_INDEX = UINT32_MAX;

		RenderGraphPassSpecifications Specifications;
		Ref<RenderGraphPass> Pass = nullptr;
		LayoutTransitionsRange Transitions;

		uint32_t RenderTargetHandleIndex = INVALID_TARGET_INDEX;
	};
}
