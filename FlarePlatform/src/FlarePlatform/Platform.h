#pragma once

#include "Flare/Core/Core.h"

#include "FlarePlatform/Window.h"

#include <filesystem>
#include <optional>

namespace Flare
{
	class Platform
	{
	public:
		static float GetTime();

		static void* LoadSharedLibrary(const std::filesystem::path& path);
		static void FreeSharedLibrary(void* library);
		static void* LoadFunction(void* library, const std::string& name);

		static std::optional<std::filesystem::path> ShowOpenFileDialog(const wchar_t* filter, const Ref<Window>& window);
		static std::optional<std::filesystem::path> ShowSaveFileDialog(const wchar_t* filter, const Ref<Window>& window);
	};
}