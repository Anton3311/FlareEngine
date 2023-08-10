#pragma once

#include "Flare/Core/Core.h"

namespace Flare
{
	class FLARE_API GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Initialize() = 0;
		virtual void SwapBuffers() = 0;
	public:
		static Scope<GraphicsContext> Create(void* windowHandle);
	};
}