#pragma once

#include "Flare/Core/UUID.h"
#include "FlareECS/World.h"

#include <filesystem>

namespace Flare
{
	class FLARE_API ScriptingEngine
	{
	public:
		struct Data
		{
			World* CurrentWorld = nullptr;
			std::vector<void*> LoadedSharedLibraries;
		};
	public:
		static void Initialize();
		static void Shutdown();

		static void SetCurrentECSWorld(World& world);

		static void LoadModules();

		static void UnloadAllModules();
		static void RegisterSystems();
		
		static Data& GetData();
	};
}