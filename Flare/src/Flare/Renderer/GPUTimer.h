#pragma once

#include "FlareCore/Core.h"

#include <optional>

namespace Flare
{
	class GPUTimer
	{
	public:
		virtual ~GPUTimer() {}

		virtual void Start() = 0;
		virtual void Stop() = 0;

		virtual std::optional<float> GetElapsedTime() = 0;
	public:
		static Ref<GPUTimer> Create();
	};
}
