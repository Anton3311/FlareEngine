#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPassSpecifications.h"

#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"

namespace Flare
{
	class RenderGraphPass;
	struct RenderPassNode
	{
		RenderGraphPassSpecifications Specifications;
		Ref<RenderGraphPass> Pass = nullptr;
	};
}
