#pragma once

#include "Flare/AssetManager/Asset.h"

#include <vector>
#include <filesystem>

namespace Flare
{
	class ShaderCompiler
	{
	public:
		static bool Compile(AssetHandle shaderHandle);
	};
}