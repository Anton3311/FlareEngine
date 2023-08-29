#include "SystemsManager.h"

#include "FlareECS/World.h"
#include "FlareECS/System/SystemInitializer.h"

namespace Flare
{
	SystemsManager::SystemsManager(World& world)
		: m_CommandBuffer(world) {}

	SystemsManager::~SystemsManager()
	{
		for (System* system : m_ManagedSystems)
			delete system;
	}

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

	SystemId SystemsManager::RegisterSystem(std::string_view name, SystemGroupId group, System* system)
	{
		SystemId id = RegisterSystem(name, group, [system](SystemExecutionContext& context)
		{
			system->OnUpdate(context);
		});

		m_ManagedSystems.push_back(system);
		return id;
	}

	SystemId SystemsManager::RegisterSystem(std::string_view name, SystemGroupId group,
		const SystemEventFunction& onUpdate)
	{
		SystemId id = RegisterSystem(name, onUpdate);
		AddSystemToGroup(id, group);
		AddSystemExecutionSettings(id, nullptr);

		return id;
	}

	SystemId SystemsManager::RegisterSystem(std::string_view name, 
		const SystemEventFunction& onUpdate)
	{
		SystemId id = (SystemId) m_Systems.size();

		SystemData& data = m_Systems.emplace_back();
		data.Name = name;
		data.Id = id;
		data.GroupId = UINT32_MAX;
		data.OnUpdate = onUpdate;
		
		return id;
	}

	void SystemsManager::RegisterSystems(SystemGroupId defaultGroup)
	{
		auto& initializers = SystemInitializer::GetInitializers();

		std::vector<std::pair<SystemId, System*>> instances(initializers.size());
		std::vector<SystemConfig> configs(initializers.size());

		size_t instanceIndex = 0;
		for (SystemInitializer* initializer : initializers)
		{
			System* instance = initializer->CreateSystem();

			SystemId id = RegisterSystem(initializer->TypeName, [instance](SystemExecutionContext& context)
			{
				instance->OnUpdate(context);
			});

			instances[instanceIndex++] = { id, instance };
			initializer->m_Id = id;
		}
		
		instanceIndex = 0;
		for (auto& [id, system] : instances)
		{
			SystemConfig& config = configs[instanceIndex];
			config.Group = {};
			system->OnConfig(config);

			AddSystemToGroup(id, config.Group.value_or(defaultGroup));
			instanceIndex++;
		}

		for (size_t i = 0; i < configs.size(); i++)
			AddSystemExecutionSettings(instances[i].first, &configs[i].GetExecutionOrder());
	}

	void SystemsManager::AddSystemToGroup(SystemId system, SystemGroupId group)
	{
		FLARE_CORE_ASSERT(m_Systems[system].GroupId == UINT32_MAX, "System is already assigned to a group");

		SystemData& data = m_Systems[system];

		m_Groups[group].SystemIndices.push_back(data.Id);

		data.GroupId = group;
		data.IndexInGroup = (uint32_t)m_Groups[group].SystemIndices.size() - 1;
	}

	void SystemsManager::AddSystemExecutionSettings(SystemId system, const std::vector<ExecutionOrder>* executionOrder)
	{
		SystemData& data = m_Systems[system];

		if (executionOrder == nullptr || executionOrder != nullptr && executionOrder->size() == 0)
			m_Groups[data.GroupId].Graph.AddExecutionSettings();
		else
		{
			std::vector<ExecutionOrder> order = *executionOrder;
			for (auto& i : order)
				i.ItemIndex = m_Systems[i.ItemIndex].IndexInGroup;

			m_Groups[data.GroupId].Graph.AddExecutionSettings(std::move(order));
		}
	}

	void SystemsManager::ExecuteGroup(SystemGroupId id)
	{
		FLARE_CORE_ASSERT(id < (SystemGroupId)m_Groups.size());

		SystemGroup& group = m_Groups[id];
		for (size_t i : group.Graph.GetExecutionOrder())
		{
			SystemData& data = m_Systems[group.SystemIndices[i]];
			data.ExecutionContext.Commands = &m_CommandBuffer;
			
			if (data.OnUpdate)
			{
				data.OnUpdate(data.ExecutionContext);
				m_CommandBuffer.Execute();
			}
		}
	}

	bool SystemsManager::IsGroupIdValid(SystemGroupId id)
	{
		return id < (SystemGroupId)m_Groups.size();
	}

	void SystemsManager::RebuildExecutionGraphs()
	{
		for (SystemGroup& group : m_Groups)
		{
			if (group.Graph.RebuildGraph() == ExecutionGraph::BuildResult::CircularDependecy)
			{
				FLARE_CORE_ERROR("Failed to build an execution graph for '{0}' because of circular dependecy", group.Name);
				continue;
			}
		}
	}

	const std::vector<SystemGroup>& SystemsManager::GetGroups() const
	{
		return m_Groups;
	}

	std::vector<SystemGroup>& SystemsManager::GetGroups()
	{
		return m_Groups;
	}

	const std::vector<SystemData>& SystemsManager::GetSystems() const
	{
		return m_Systems;
	}
}
