#include "Event.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/Components.h"
#include "FlareECS/Entity/ComponentInitializer.h"

namespace Flare
{
	EventStorage::EventStorage(const Components& components, ComponentId eventType)
		: m_Components(components), m_EventType(eventType)
	{
		FLARE_CORE_ASSERT(m_Components.IsComponentIdValid(eventType));

		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(m_EventType);
		m_EventsPerChunk = EntityStorageChunk::CHUNK_SIZE / componentInfo.Size;
		m_EventSize = componentInfo.Size;
	}

	EventStorage::~EventStorage()
	{
		FLARE_PROFILE_FUNCTION();

		Clear();
	}

	void EventStorage::AppendEventCopy(const uint8_t* eventData)
	{
		FLARE_PROFILE_FUNCTION();
		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(m_EventType);

		FLARE_CORE_VERIFY(componentInfo.Initializer->Type.Functions.CopyConstructor);

		uint8_t* destination = AllocateEvent(componentInfo);
		componentInfo.Initializer->Type.Functions.CopyConstructor(destination, eventData);
	}

	void EventStorage::AppendEvent(uint8_t* eventData)
	{
		FLARE_PROFILE_FUNCTION();
		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(m_EventType);

		FLARE_CORE_VERIFY(componentInfo.Initializer->Type.Functions.MoveConstructor);

		uint8_t* destination = AllocateEvent(componentInfo);
		componentInfo.Initializer->Type.Functions.MoveConstructor(destination, eventData);
	}

	void EventStorage::Clear()
	{
		FLARE_PROFILE_FUNCTION();

		const ComponentInfo& componentInfo = m_Components.GetComponentInfo(m_EventType);
		for (size_t chunkIndex = 0; chunkIndex < m_Chunks.size(); chunkIndex++)
		{
			size_t eventCountInChunk = m_EventsPerChunk;

			// Only the last chunk is partially filled
			if (chunkIndex == m_Chunks.size() - 1)
				eventCountInChunk = m_EventCount % m_EventsPerChunk;

			uint8_t* chunkData = m_Chunks[chunkIndex].GetBuffer();
			for (size_t i = 0; i < eventCountInChunk; i++)
			{
				componentInfo.Initializer->Type.Functions.Destructor(chunkData + i * componentInfo.Size);
			}

			EntityChunksPool::GetInstance()->Add(std::move(m_Chunks[chunkIndex]));
		}

		m_Chunks.clear();
		m_EventCount = 0;
	}

	uint8_t* EventStorage::AllocateEvent(const ComponentInfo& componentInfo)
	{
		FLARE_PROFILE_FUNCTION();

		if (m_EventCount % m_EventsPerChunk == 0)
			m_Chunks.push_back(EntityChunksPool::GetInstance()->GetOrCreate());

		size_t lastChunkEventCount = m_EventCount % m_EventsPerChunk;
		uint8_t* destination = m_Chunks.back().GetBuffer() + lastChunkEventCount * componentInfo.Size;

		m_EventCount++;

		return destination;
	}
}
