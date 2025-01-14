#include "EventManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/Components.h"

namespace Flare
{
	EventManager::EventManager(const Components& components)
		: m_Components(components)
	{
	}

	EventManager::~EventManager()
	{
	}

	void EventManager::ReleaseCollectedEvents()
	{
		FLARE_PROFILE_FUNCTION();

		for (auto& [eventType, eventStorage] : m_EventStorages)
		{
			eventStorage.Clear();
		}
	}

	EventStorage& Flare::EventManager::GetOrCreateEventStorage(ComponentId eventType)
	{
		FLARE_PROFILE_FUNCTION();
		auto it = m_EventStorages.find(eventType);
		if (it == m_EventStorages.end())
		{
			FLARE_CORE_ASSERT(m_Components.IsComponentIdValid(eventType));
			return m_EventStorages.emplace(eventType, EventStorage(m_Components, eventType)).first->second;
		}

		return it->second;
	}
}
