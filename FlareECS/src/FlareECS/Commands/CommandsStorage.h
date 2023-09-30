#pragma once

#include "FlareCore/Core.h"
#include "FlareECS/Commands/Command.h"

#include <stdint.h>

namespace Flare
{
	class FLAREECS_API CommandsStorage
	{
	public:
		CommandsStorage(size_t capacity);
		~CommandsStorage();

		void* Allocate(const CommandMetadata& meta);
		std::pair<const CommandMetadata&, Command*> Pop();
		bool CanRead();
		void Clear();
	private:
		void Reallocate();
	private:
		uint8_t* m_Buffer;
		size_t m_Size;
		size_t m_Capacity;

		size_t m_ReadPosition;
	};
}