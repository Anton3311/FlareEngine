#pragma once

#include "Flare/Core/Core.h"

#ifdef FLARE_PLATFORM_WINDOWS

#include "FlareScripting/ScriptingModule.h"

#include <windows.h>

namespace Flare
{
	class WindowsScriptingModule : public ScriptingModule
	{
	public:
		WindowsScriptingModule(const std::filesystem::path& path);
		virtual ~WindowsScriptingModule();

		virtual bool IsLoaded() override;

		virtual std::optional<ScriptingModuleFunction> LoadFunction(const std::string& name) override;
	private:
		HMODULE m_Module;
		bool m_Loaded;
	};
}

#endif