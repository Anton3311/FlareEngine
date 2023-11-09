#pragma once

#include "FlareEditor/ShaderCompiler/ShaderSyntax.h"

#include <string>

namespace Flare
{
	struct ShaderError
	{
		ShaderError(SourcePosition position, const std::string& message)
			: Position(position), Message(message) {}

		SourcePosition Position;
		std::string Message;
	};
}