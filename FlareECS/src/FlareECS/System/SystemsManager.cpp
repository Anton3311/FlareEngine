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
		auto it = m_GroupNameToId.find(name);
		if (it == m_GroupNameToId.end())
			return {};
		return it->second;
	}

	void SystemsManager::RegisterSystem(std::string_view name, 
		SystemGroupId group,
		const Query& query,
		const SystemEventFunction& onBeforeUpdate,
		const SystemEventFunction& onUpdate,
		const SystemEventFunction& onAfterUpdate)
	{
		uint32_t index = (uint32_t) m_Systems.size();

		SystemData& data = m_Systems.emplace_back(query);
		data.Name = name;
		data.Index = index;
		data.OnUpdate = onUpdate;
		data.OnBeforeUpdate = onBeforeUpdate;
		data.OnAfterUpdate = onAfterUpdate;

		FLARE_CORE_ASSERT(group < (SystemGroupId)m_Groups.size());
		m_Groups[group].SystemIndices.push_back(index);
		data.IndexInGroup = (uint32_t) m_Groups[group].SystemIndices.size() - 1;
	}

	void SystemsManager::ExecuteGroup(SystemGroupId id)
	{
		FLARE_CORE_ASSERT(id < (SystemGroupId)m_Groups.size());

		SystemGroup group = m_Groups[id];
		for (size_t i = 0; i < group.SystemIndices.size(); i++)
		{
			SystemData& data = m_Systems[i];
			if (data.OnBeforeUpdate)
				data.OnBeforeUpdate(data.ExecutionContext);

			if (data.OnUpdate)
				data.OnUpdate(data.ExecutionContext);

			if (data.OnAfterUpdate)
				data.OnAfterUpdate(data.ExecutionContext);
		}
	}
}
