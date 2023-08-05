#pragma once

#include "FlareECS/Query/Query.h"
#include "FlareECS/System.h"

#include "FlareECS/System/System.h"
#include "FlareECS/System/SystemData.h"

#include <functional>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Flare
{
	class System;
	class SystemsManager
	{
	public:
		SystemsManager() = default;

		SystemGroupId CreateGroup(std::string_view name);
		std::optional<SystemGroupId> FindGroup(std::string_view name) const;

		SystemId RegisterSystem(std::string_view name, SystemGroupId group, Scope<System> system)
		{
			SystemId id = RegisterSystem(name, group, [system = system.get()](SystemExecutionContext& context)
			{
				system->OnUpdate(context);
			});

			m_ManagedSystems.push_back(std::move(system));
			return id;
		}

		SystemId RegisterSystem(std::string_view name, SystemGroupId group,
			const SystemEventFunction& onUpdate = nullptr);

		SystemId RegisterSystem(std::string_view name, 
			const SystemEventFunction& onUpdate = nullptr);

		void AddSystemToGroup(SystemId system, SystemGroupId group);
		void AddSystemExecutionSettings(SystemId system, const std::vector<ExecutionOrder>* executionOrder);

		void ExecuteGroup(SystemGroupId id);
		inline bool IsGroupIdValid(SystemGroupId id)
		{
			return id < (SystemGroupId)m_Groups.size();
		}

		void RebuildExecutionGraphs();
		
		inline const std::vector<SystemGroup>& GetGroups() const { return m_Groups; }
		inline std::vector<SystemGroup>& GetGroups() { return m_Groups; }

		inline const std::vector<SystemData>& GetSystems() const { return m_Systems; }
	private:
		std::vector<SystemData> m_Systems;
		std::unordered_map<std::string, SystemGroupId> m_GroupNameToId;
		std::vector<SystemGroup> m_Groups;

		std::vector<Scope<System>> m_ManagedSystems;
	};
}