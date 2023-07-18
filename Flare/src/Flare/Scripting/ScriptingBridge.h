#pragma once

#include "FlareECS/World.h"

#include "FlareScriptingCore/ModuleConfiguration.h"

namespace Flare
{
	class ScriptingBridge
	{
	public:
		static void ConfigureModule(ModuleConfiguration& config);

		inline static void SetCurrentWorld(World& world)
		{
			s_World = &world;
		}
	private:
		static World* s_World;
	};
}