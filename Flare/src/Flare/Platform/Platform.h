#pragma once

#include <filesystem>
#include <optional>

namespace Flare
{
	class Platform
	{
	public:
		static std::optional<std::filesystem::path> ShowOpenFileDialog(const wchar_t* filter);
		static std::optional<std::filesystem::path> ShowSaveFileDialog(const wchar_t* filter);
	};
}