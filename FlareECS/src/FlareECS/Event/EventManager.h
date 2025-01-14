#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Event/Event.h"

#include <unordered_map>

namespace Flare
{
	class FLAREECS_API EventManager
	{
	public:
		FLARE_NONCOPYABLE(EventManager);

		EventManager(const Components& components);
		~EventManager();

		template<typename T>
		EventWriter<T> GetEventWriter()
		{
			ComponentId eventType = COMPONENT_ID(T);
			return GetOrCreateEventStorage(eventType).CreateWriter<T>();
		}

		inline const EventStorage* TryGetEventStorage(ComponentId eventType) const
		{
			auto it = m_EventStorages.find(eventType);
			if (it == m_EventStorages.end())
				return nullptr;
			return &it->second;
		}

		void ReleaseCollectedEvents();
	private:
		EventStorage& GetOrCreateEventStorage(ComponentId eventType);
	private:
		const Components& m_Components;
		std::unordered_map<ComponentId, EventStorage> m_EventStorages;
	};
}
