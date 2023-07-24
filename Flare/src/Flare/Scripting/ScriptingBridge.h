#pragma once

#include "FlareECS/World.h"

#include "FlareScriptingCore/ModuleConfiguration.h"

namespace Flare
{
	class ScriptingBridge
	{
	public:
		static void ConfigureModule(Internal::ModuleConfiguration& config);

		inline static World& GetCurrentWorld();

		inline static void SetCurrentWorld(World& world)
		{
			s_World = &world;
		}
	private:
		static World* s_World;
	};
}