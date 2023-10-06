#pragma once

#include "FlareCore/Core.h"
#include "FlareECS/Commands/Command.h"

#include <stdint.h>
#include <optional>

namespace Flare
{
	struct CommandAllocation
	{
		size_t MetaLocation;
		size_t CommandLocation;
	};

	class FLAREECS_API CommandsStorage
	{
	public:
		CommandsStorage(size_t capacity);
		~CommandsStorage();

		template<typename T>
		std::optional<const T*> Read(size_t location) const
		{
			if (location + sizeof(T) <= m_Capacity)
				return (const T*)(m_Buffer + location);
			return {};
		}

		template<typename T>
		std::optional<T*> Read(size_t location)
		{
			if (location + sizeof(T) <= m_Capacity)
				return (T*)(m_Buffer + location);
			return {};
		}

		template<typename T>
		bool Write(size_t location, const T& value)
		{
			if (location + sizeof(T) <= m_Capacity)
			{
				new(m_Buffer + location) T;
				*(T*)(m_Buffer + location) = value;
				return true;
			}
			return false;
		}

		std::optional<size_t> Allocate(size_t size);

		std::optional<CommandAllocation> AllocateCommand(size_t commandSize);
		std::pair<CommandMetadata&, Command*> Pop();

		inline size_t GetReadPosition() const { return m_ReadPosition; }
		inline size_t GetSize() const { return m_Size; }

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