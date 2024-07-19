#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	class FLARE_API ScriptingEngine
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void LoadModules();
		static void UnloadAllModules();
	};
}