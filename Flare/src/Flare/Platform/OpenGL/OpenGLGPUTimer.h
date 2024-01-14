#pragma once

#include "Flare/Renderer/GPUTimer.h"

#include <optional>

namespace Flare
{
	class OpenGLGPUTImer : public GPUTimer
	{
	public:
		OpenGLGPUTImer();
		~OpenGLGPUTImer();

		void Start() override;
		void Stop() override;
		std::optional<float> GetElapsedTime() override;
	private:
		uint32_t m_Id;
	};
}
