#pragma once

#include "Flare/Core/Core.h"

#include "FlarePlatform/Window.h"

#include <filesystem>
#include <optional>

#undef CreateProcess

namespace Flare
{
	struct ProcessCreationSettings
	{
		uint32_t MillisecondsTimeout = 0xffffffff;
		std::wstring Arguments;
		std::filesystem::path WorkingDirectory = std::filesystem::current_path();
	};

	class FLAREPLATFORM_API Platform
	{
	public:
		static float GetTime();

		static void* LoadSharedLibrary(const std::filesystem::path& path);
		static void FreeSharedLibrary(void* library);
		static void* LoadFunction(void* library, const std::string& name);

		static int32_t CreateProcess(std::filesystem::path& path, const ProcessCreationSettings& settings);

		static std::optional<std::filesystem::path> ShowOpenFileDialog(const wchar_t* filter, const Ref<Window>& window);
		static std::optional<std::filesystem::path> ShowSaveFileDialog(const wchar_t* filter, const Ref<Window>& window);
	};
}