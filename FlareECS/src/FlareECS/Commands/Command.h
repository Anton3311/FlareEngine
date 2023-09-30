#pragma once

#include <stdint.h>

namespace Flare
{
	class FLAREECS_API World;
	class FLAREECS_API Command
	{
	public:
		virtual ~Command() = default;
		virtual void Apply(World& world) = 0;
	};

	using ApplyCommandFunction = void(*)(Command*, World&);

	struct CommandMetadata
	{
		size_t CommandSize;
	};
}
