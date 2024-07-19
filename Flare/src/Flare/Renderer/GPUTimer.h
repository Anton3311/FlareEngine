#pragma once

#include "FlareCore/Core.h"

#include <optional>

namespace Flare
{
	class GPUTimer
	{
	public:
		virtual ~GPUTimer() {}

		virtual std::optional<float> GetElapsedTime() = 0;
	public:
		static Ref<GPUTimer> Create();
	};
}
