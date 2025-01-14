#pragma once

#include "FlareECS/Entity/Component.h"
#include "FlareECS/Entity/ComponentInitializer.h"
#include "FlareECS/EntityStorage/EntityStorageChunk.h"

#include <vector>

namespace Flare
{
	struct Components;
	class EventStorage;

	//
	// EventWriter
	//

	template<typename T>
	class EventWriter
	{
	private:
		explicit EventWriter(EventStorage& storage);
	public:
		inline void Append(T&& event);
		inline void Append(const T& event);
	private:
		EventStorage& m_EventStorage;

		friend EventStorage;
	};

	//
	// EventReader
	//

	template<typename T>
	class EventStorageIterator
	{
	public:
		constexpr EventStorageIterator(const EventStorage& storage, size_t eventIndex)
			: m_Storage(storage), m_EventIndex(eventIndex) {}

		constexpr bool operator==(const EventStorageIterator& other) const
		{
			return &m_Storage == &other.m_Storage && m_EventIndex == other.m_EventIndex;
		}
			
		constexpr bool operator!=(const EventStorageIterator& other) const
		{
			return &m_Storage != &other.m_Storage || m_EventIndex != other.m_EventIndex;
		}

		constexpr EventStorageIterator<T> operator++();
		inline const T& operator*() const;
	private:
		const EventStorage& m_Storage;
		size_t m_EventIndex = 0;
	};

	template<typename T>
	class EventReader
	{
	private:
		explicit EventReader(const EventStorage& storage);
	public:
		inline EventStorageIterator<T> begin() const;
		inline EventStorageIterator<T> end() const;
	private:
		const EventStorage& m_Storage;

		friend class EventStorage;
	};

	//
	// EventStorage
	//

	class FLAREECS_API EventStorage
	{
	public:
		FLARE_NONCOPYABLE(EventStorage);

		EventStorage(const Components& components, ComponentId eventType);
		~EventStorage();

		EventStorage(EventStorage&&) = default;
		EventStorage& operator=(EventStorage&&) = default;

		constexpr ComponentId GetEventType() const { return m_EventType; }
		constexpr size_t GetEventCount() const { return m_EventCount; }
		constexpr size_t GetChunkCount() const { return m_Chunks.size(); }
		constexpr size_t GetEventCountPerChunk() const { return m_EventsPerChunk; }

		void AppendEventCopy(const uint8_t* eventData);
		void AppendEvent(uint8_t* eventData);

		void Clear();

		template<typename T>
		EventWriter<T> CreateWriter()
		{
			return EventWriter<T>(*this);
		}

		template<typename T>
		EventReader<T> CreateReader() const
		{
			return EventReader<T>(*this);
		}
	private:
		uint8_t* AllocateEvent(const ComponentInfo& componentInfo);
	private:
		const Components& m_Components;
		ComponentId m_EventType = ComponentId();

		size_t m_EventsPerChunk = 0;
		size_t m_EventCount = 0;
		size_t m_EventSize = 0;
		std::vector<EntityStorageChunk> m_Chunks;

		template<typename T>
		friend class EventStorageIterator;
	};

	//
	// EventWriter
	//

	template<typename T>
	inline EventWriter<T>::EventWriter(EventStorage& storage)
		: m_EventStorage(storage)
	{
		ComponentId id = COMPONENT_ID(T);
		FLARE_CORE_VERIFY(id == m_EventStorage.GetEventType());
	}

	template<typename T>
	inline void EventWriter<T>::Append(T&& event)
	{
		m_EventStorage.AppendEvent(reinterpret_cast<uint8_t*>(&event));
	}

	template<typename T>
	inline void EventWriter<T>::Append(const T& event)
	{
		m_EventStorage.AppendEventCopy(reinterpret_cast<const uint8_t*>(&event));
	}

	template<typename T>
	inline constexpr EventStorageIterator<T> EventStorageIterator<T>::operator++()
	{
		m_EventIndex++;
		return *this;
	}

	template<typename T>
	inline const T& EventStorageIterator<T>::operator*() const
	{
		size_t chunkIndex = m_EventIndex / m_Storage.GetEventCountPerChunk();
		size_t indexInChunk = m_EventIndex % m_Storage.GetEventCountPerChunk();

		const uint8_t* eventData = m_Storage.m_Chunks[chunkIndex].GetBuffer() + indexInChunk * m_Storage.m_EventSize;
		return *reinterpret_cast<const T*>(eventData);
	}

	//
	// EventReader
	//

	template<typename T>
	inline EventReader<T>::EventReader(const EventStorage& storage)
		: m_Storage(storage)
	{
		ComponentId id = COMPONENT_ID(T);
		FLARE_CORE_VERIFY(id == m_Storage.GetEventType());
	}

	template<typename T>
	inline EventStorageIterator<T> EventReader<T>::begin() const
	{
		return EventStorageIterator<T>(m_Storage, 0);
	}

	template<typename T>
	inline EventStorageIterator<T> EventReader<T>::end() const
	{
		return EventStorageIterator<T>(m_Storage, m_Storage.GetEventCount());
	}
}
