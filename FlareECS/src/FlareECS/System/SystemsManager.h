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
	class FLAREECS_API SystemsManager
	{
	public:
		SystemsManager() = default;
		~SystemsManager();

		SystemGroupId CreateGroup(std::string_view name);
		std::optional<SystemGroupId> FindGroup(std::string_view name) const;

		SystemId RegisterSystem(std::string_view name, SystemGroupId group, System* system);
		SystemId RegisterSystem(std::string_view name, SystemGroupId group, const SystemEventFunction& onUpdate = nullptr);
		SystemId RegisterSystem(std::string_view name,  const SystemEventFunction& onUpdate = nullptr);

		void AddSystemToGroup(SystemId system, SystemGroupId group);
		void AddSystemExecutionSettings(SystemId system, const std::vector<ExecutionOrder>* executionOrder);

		void ExecuteGroup(SystemGroupId id);
		bool IsGroupIdValid(SystemGroupId id);
		void RebuildExecutionGraphs();
		
		const std::vector<SystemGroup>& GetGroups() const;
		std::vector<SystemGroup>& GetGroups();

		const std::vector<SystemData>& GetSystems() const;
	private:
		std::vector<SystemData> m_Systems;
		std::unordered_map<std::string, SystemGroupId> m_GroupNameToId;
		std::vector<SystemGroup> m_Groups;

		std::vector<System*> m_ManagedSystems;
	};
}