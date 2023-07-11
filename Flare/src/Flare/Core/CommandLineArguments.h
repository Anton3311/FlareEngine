#pragma once

#include <stdint.h>

namespace Flare
{
	struct CommandLineArguments
	{
		const char** Arguments = nullptr;
		uint32_t ArgumentsCount = 0;

		const char* operator[](uint32_t index)
		{
			return Arguments[index];
		}
	};
}