#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	class FLARE_API GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Initialize() = 0;
		virtual void SwapBuffers() = 0;
		virtual void OnWindowResize() = 0;

	public:
		static GraphicsContext& GetInstance();
		static void Create(void* windowHandle);
		static void Shutdown();
	};
}