#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPassSpecifications.h"

#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"

namespace Flare
{
	class RenderGraphPass;
	struct RenderPassNode
	{
		static constexpr uint32_t INVALID_TARGET_INDEX = UINT32_MAX;

		RenderGraphPassSpecifications Specifications;
		Ref<RenderGraphPass> Pass = nullptr;
		LayoutTransitionsRange Transitions;

		uint32_t RenderTargetHandleIndex = INVALID_TARGET_INDEX;
	};
}
