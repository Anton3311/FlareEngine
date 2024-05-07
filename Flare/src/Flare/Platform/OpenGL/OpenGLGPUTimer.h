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

		void Start();
		void Stop();

		std::optional<float> GetElapsedTime() override;
		uint32_t GetId() const { return m_Id; }
	private:
		uint32_t m_Id;
	};
}
