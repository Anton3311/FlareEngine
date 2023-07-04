#pragma once

#include "FlareECS/Registry.h"
#include "FlareECS/Entity/Component.h"

#include "FlareECS/System/System.h"

#include <vector>
#include <string_view>

namespace Flare
{
	class World
	{
	public:
		inline Registry& GetRegistry() { return m_Registry; }

		void RegisterSystem(QueryId query, const SystemFunction& system);

		void OnUpdate();
	private:
		Registry m_Registry;

		std::vector<System> m_Systems;
	};
}