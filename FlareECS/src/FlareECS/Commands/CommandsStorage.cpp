#include "CommandsStorage.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	CommandsStorage::CommandsStorage(size_t capacity)
		: m_Capacity(capacity), m_Size(0), m_Buffer(nullptr), m_ReadPosition(0)
	{
	}

	CommandsStorage::~CommandsStorage()
	{
		if (m_Buffer != nullptr)
			delete[] m_Buffer;
	}

	std::optional<size_t> CommandsStorage::Allocate(size_t size)
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Buffer == nullptr || m_Size + size > m_Capacity)
			Reallocate();

		if (m_Buffer != nullptr)
		{
			size_t offset = m_Size;
			m_Size += size;
			return offset;
		}

		return {};
	}

	std::optional<CommandAllocation> CommandsStorage::AllocateCommand(size_t commandSize)
	{
		size_t itemSize = sizeof(CommandMetadata) + commandSize;
		size_t newSize = m_Size + itemSize;

		if (m_Buffer == nullptr || newSize > m_Capacity)
			Reallocate();

		if (m_Buffer != nullptr)
		{
			size_t oldSize = m_Size;
			m_Size = newSize;

			return { { oldSize, oldSize + sizeof(CommandMetadata) } };
		}

		return {};
	}

	std::pair<CommandMetadata&, Command*> CommandsStorage::Pop()
	{
		FLARE_CORE_ASSERT(m_ReadPosition + sizeof(CommandMetadata) <= m_Size);
		CommandMetadata& metadata = *(CommandMetadata*)(m_Buffer + m_ReadPosition);

		FLARE_CORE_ASSERT(m_ReadPosition + sizeof(CommandMetadata) + metadata.CommandSize <= m_Size);
		void* command = m_Buffer + m_ReadPosition + sizeof(CommandMetadata);

		m_ReadPosition += sizeof(CommandMetadata) + metadata.CommandSize;
		return { metadata, (Command*) command };
	}

	bool CommandsStorage::CanRead()
	{
		return m_ReadPosition < m_Size;
	}

	void CommandsStorage::Clear()
	{
		m_ReadPosition = 0;
		m_Size = 0;

		if (m_Buffer)
			std::memset(m_Buffer, 0, m_Capacity);
	}

	void CommandsStorage::Reallocate()
	{
		FLARE_PROFILE_FUNCTION();
		m_Capacity *= 2;
		uint8_t* newBuffer = new uint8_t[m_Capacity];

		if (m_Buffer != nullptr)
		{
			errno_t error = memcpy_s(newBuffer, m_Capacity, m_Buffer, m_Size);
			FLARE_CORE_ASSERT(error == 0);
			
			delete[] m_Buffer;
		}

		m_Buffer = newBuffer;
	}
}
