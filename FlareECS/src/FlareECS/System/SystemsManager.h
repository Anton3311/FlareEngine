#pragma once

#include "FlareECS/Query/Query.h"
#include "FlareECS/System.h"

#include "FlareECS/System/SystemData.h"

#include <functional>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Flare
{
	class SystemsManager
	{
	public:
		SystemGroupId CreateGroup(std::string_view name);
		std::optional<SystemGroupId> FindGroup(std::string_view name) const;

		SystemId RegisterSystem(std::string_view name, 
			SystemGroupId group, const Query& query,
			const SystemEventFunction& onBeforeUpdate = nullptr,
			const SystemEventFunction& onUpdate = nullptr,
			const SystemEventFunction& onAfterUpdate = nullptr);

		SystemId RegisterSystem(std::string_view name, 
			const SystemEventFunction& onBeforeUpdate = nullptr,
			const SystemEventFunction& onUpdate = nullptr,
			const SystemEventFunction& onAfterUpdate = nullptr);

		void AddSystemToGroup(SystemId system, SystemGroupId group);
		void AddSystemExecutionSettings(SystemId system, const std::vector<ExecutionOrder>* executionOrder);

		// Systems should own queries, be able to create them on configuration and use them to iterate over entities
		void SetSystemQuery(SystemId system, const Query& query);

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
	};
}