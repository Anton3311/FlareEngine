#include "SystemsManager.h"

namespace Flare
{
	SystemGroupId SystemsManager::CreateGroup(std::string_view name)
	{
		SystemGroupId id = (SystemGroupId)m_Groups.size();
		SystemGroup& group = m_Groups.emplace_back();
		group.Id = id;
		group.Name = name;

		m_GroupNameToId.emplace(group.Name, id);
		return id;
	}

	std::optional<SystemGroupId> SystemsManager::FindGroup(std::string_view name) const
	{
		auto it = m_GroupNameToId.find(std::string(name));
		if (it == m_GroupNameToId.end())
			return {};
		return it->second;
	}

	uint32_t SystemsManager::RegisterSystem(std::string_view name, 
		const SystemEventFunction& onBeforeUpdate,
		const SystemEventFunction& onUpdate,
		const SystemEventFunction& onAfterUpdate)
	{
		uint32_t index = (uint32_t) m_Systems.size();

		SystemData& data = m_Systems.emplace_back();
		data.Name = name;
		data.Id = index;
		data.GroupId = UINT32_MAX;
		data.OnUpdate = onUpdate;
		data.OnBeforeUpdate = onBeforeUpdate;
		data.OnAfterUpdate = onAfterUpdate;
		
		return index;
	}

	void SystemsManager::AddSystemToGroup(SystemId system, SystemGroupId group)
	{
		FLARE_CORE_ASSERT(m_Systems[system].GroupId == UINT32_MAX, "System is already assign to a group");

		SystemData& data = m_Systems[system];

		m_Groups[group].SystemIndices.push_back(data.Id);

		data.GroupId = group;
		data.IndexInGroup = (uint32_t)m_Groups[group].SystemIndices.size() - 1;
	}

	void SystemsManager::AddSystemExecutionSettings(SystemId system, const std::vector<ExecutionOrder>* executionOrder)
	{
		SystemData& data = m_Systems[system];

		if (executionOrder == nullptr)
			m_Groups[data.GroupId].Graph.AddExecutionSettings();
		else
		{
			auto a = *executionOrder;
			for (auto& order : a)
				order.ItemIndex = m_Systems[order.ItemIndex].IndexInGroup;

			m_Groups[data.GroupId].Graph.AddExecutionSettings(a);
		}
	}

	void SystemsManager::SetSystemQuery(SystemId system, const Query& query)
	{
		FLARE_CORE_ASSERT(system < (SystemId)m_Systems.size());
		m_Systems[system].ExecutionContext.SetQuery(query);
	}

	void SystemsManager::ExecuteGroup(SystemGroupId id)
	{
		FLARE_CORE_ASSERT(id < (SystemGroupId)m_Groups.size());

		SystemGroup& group = m_Groups[id];
		for (size_t i : group.Graph.GetExecutionOrder())
		{
			SystemData& data = m_Systems[group.SystemIndices[i]];
			if (data.OnBeforeUpdate)
				data.OnBeforeUpdate(data.ExecutionContext);

			if (data.OnUpdate)
				data.OnUpdate(data.ExecutionContext);

			if (data.OnAfterUpdate)
				data.OnAfterUpdate(data.ExecutionContext);
		}
	}

	void SystemsManager::RebuildExecutionGraphs()
	{
		for (SystemGroup& group : m_Groups)
		{
			if (group.Graph.RebuildGraph() == ExecutionGraph::BuildResult::CircularDependecy)
			{
				FLARE_CORE_ERROR("Faield to build an execution graph for '{0}' because of circular dependecy", group.Name);
				continue;
			}
		}
	}
}
