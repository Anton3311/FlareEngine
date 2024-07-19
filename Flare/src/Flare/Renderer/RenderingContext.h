#pragma once

#include "Flare/Math/Math.h"

#include "Flare/Renderer/FrameBuffer.h"

namespace Flare
{
	struct RenderingContext
	{
		RenderingContext(const Ref<FrameBuffer>& renderTarget)
			: RenderTarget(renderTarget) {}

		const Ref<FrameBuffer> RenderTarget;
	};
}