#pragma once

#include "FlareECS/World.h"

#include <stdint.h>

namespace Flare
{
	class Command
	{
	public:
		virtual void Apply(World& world) = 0;
	};

	using ApplyCommandFunction = void(*)(Command*, World&);

	struct CommandMetadata
	{
		ApplyCommandFunction Apply;
		size_t CommandSize;
	};
}
