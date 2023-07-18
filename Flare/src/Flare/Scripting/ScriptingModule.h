#pragma once

#include "Flare/Core/Core.h"

#include <filesystem>
#include <optional>

namespace Flare
{
	using ScriptingModuleFunction = void(*)();

	class ScriptingModule
	{
	public:
		virtual ~ScriptingModule() {}

		virtual bool IsLoaded() = 0;

		virtual std::optional<ScriptingModuleFunction> LoadFunction(const std::string& name) = 0;
	public:
		static Ref<ScriptingModule> Create(const std::filesystem::path& path);
	};
}