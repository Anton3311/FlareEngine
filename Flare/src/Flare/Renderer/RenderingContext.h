#pragma once

#include "Flare/Math/Math.h"

#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/RenderTargetsPool.h"

namespace Flare
{
	struct RenderingContext
	{
		RenderingContext(const Ref<FrameBuffer>& renderTarget, RenderTargetsPool& rtPool)
			: RenderTarget(renderTarget), RTPool(rtPool) {}

		const Ref<FrameBuffer> RenderTarget;
		RenderTargetsPool& RTPool;
	};
}