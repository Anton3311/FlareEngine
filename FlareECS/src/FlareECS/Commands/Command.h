#pragma once

#include <stdint.h>

namespace Flare
{
	class FLAREECS_API World;
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
