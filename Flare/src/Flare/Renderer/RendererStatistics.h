#pragma once

#include <stdint.h>

namespace Flare
{
	struct RendererStatistics
	{
		uint32_t DrawCallCount = 0;
		uint32_t DrawCallsSavedByInstancing = 0;

		uint32_t ObjectsSubmitted = 0;
		uint32_t ObjectsVisible = 0;

		float ShadowPassTime = 0.0f;
		float GeometryPassTime = 0.0f;
	};
}
