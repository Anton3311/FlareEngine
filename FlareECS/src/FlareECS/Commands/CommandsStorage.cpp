#include "CommandsStorage.h"

#include "FlareCore/Assert.h"

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

	void CommandsStorage::Push(const CommandMetadata& meta, const void* commandData)
	{
		FLARE_CORE_ASSERT(commandData);

		size_t itemSize = sizeof(meta) + meta.CommandSize;
		size_t newSize = m_Size + itemSize;

		if (m_Buffer == nullptr || newSize > m_Capacity)
			Reallocate();

		if (m_Buffer != nullptr)
		{
			std::memcpy(m_Buffer + m_Size, &meta, sizeof(meta));
			std::memcpy(m_Buffer + m_Size + sizeof(meta), commandData, meta.CommandSize);

			m_Size = newSize;
		}
	}

	std::pair<const CommandMetadata&, void*> CommandsStorage::Pop()
	{
		FLARE_CORE_ASSERT(m_ReadPosition + sizeof(CommandMetadata) <= m_Size);
		const CommandMetadata& metadata = *(CommandMetadata*)(m_Buffer + m_ReadPosition);

		FLARE_CORE_ASSERT(m_ReadPosition + sizeof(CommandMetadata) + metadata.CommandSize <= m_Size);
		void* command = m_Buffer + m_ReadPosition + sizeof(CommandMetadata);

		m_ReadPosition += sizeof(CommandMetadata) + metadata.CommandSize;
		return { metadata, command };
	}

	bool CommandsStorage::CanRead()
	{
		return m_ReadPosition < m_Size;
	}

	void CommandsStorage::Clear()
	{
		m_ReadPosition = 0;
		m_Size = 0;
	}

	void CommandsStorage::Reallocate()
	{
		m_Capacity *= 2;
		uint8_t* newBuffer = new uint8_t[m_Capacity];

		if (m_Buffer != nullptr)
		{
			std::memcpy(newBuffer, m_Buffer, m_Size);
			delete[] m_Buffer;
		}

		m_Buffer = newBuffer;
	}
}
