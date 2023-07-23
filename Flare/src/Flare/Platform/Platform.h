#pragma once

#include <filesystem>
#include <optional>

namespace Flare
{
	class Platform
	{
	public:
		static void* LoadSharedLibrary(const std::filesystem::path& path);
		static void FreeSharedLibrary(void* library);
		static void* LoadFunction(void* library, const std::string& name);

		static std::optional<std::filesystem::path> ShowOpenFileDialog(const wchar_t* filter);
		static std::optional<std::filesystem::path> ShowSaveFileDialog(const wchar_t* filter);
	};
}