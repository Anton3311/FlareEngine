#pragma once

#include "Flare/Renderer/RenderingContext.h"

namespace Flare
{
	class FLARE_API RenderPass
	{
	public:
		virtual ~RenderPass() = default;
		virtual void OnRender(RenderingContext& context) = 0;
	};
}