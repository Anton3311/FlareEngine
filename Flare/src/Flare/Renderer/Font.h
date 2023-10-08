#pragma once

#include "FlareCore/Core.h"

#include <filesystem>

namespace Flare
{
	class FLARE_API Font
	{
	public:
		Font(const std::filesystem::path& path);
	};
}