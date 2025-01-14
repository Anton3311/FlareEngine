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
		constexpr size_t GetCount() const { return m_Count; }

		void AppendEventCopy(const uint8_t* eventData);
		void AppendEvent(uint8_t* eventData);

		void Clear();

		template<typename T>
		EventWriter<T> CreateWriter()
		{
			return EventWriter<T>(*this);
		}
	private:
		uint8_t* AllocateEvent(const ComponentInfo& componentInfo);
	private:
		const Components& m_Components;
		ComponentId m_EventType = ComponentId();

		size_t m_EventsPerChunk = 0;
		size_t m_Count = 0;
		std::vector<EntityStorageChunk> m_Chunks;
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
}
