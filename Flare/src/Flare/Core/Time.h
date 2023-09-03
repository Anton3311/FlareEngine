#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	class FLARE_API Application;
	class Time
	{
	public:
		FLARE_API static float GetDeltaTime();
	private:
		static void UpdateDeltaTime();

		friend class Application;
	};
}