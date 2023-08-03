#pragma once

#include "FlareECS/World.h"

#include "FlareScriptingCore/ModuleConfiguration.h"

namespace Flare
{
	class ScriptingBridge
	{
	public:
		static void Initialize();

		inline static World& GetCurrentWorld();
		inline static Internal::Bindings& GetBindings() { return s_Bindings; }

		inline static void SetCurrentWorld(World& world)
		{
			s_World = &world;
		}
	private:
		static World* s_World;
		static Internal::Bindings s_Bindings;
	};
}