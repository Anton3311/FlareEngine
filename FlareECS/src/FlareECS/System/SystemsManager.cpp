#include "SystemsManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"
#include "FlareECS/System/SystemInitializer.h"
#include "FlareECS/System/SystemsRegistry.h"

namespace Flare
{
	SystemsManager::SystemsManager(World& world, SystemsRegistry& registry)
		: m_CommandBuffer(world), m_World(world), m_Registry(registry) {}

	SystemsManager::~SystemsManager()
	{
		for (SystemData& system : m_Systems)
		{
			delete system.SystemInstance;
			system.SystemInstance = nullptr;
		}
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

	void SystemsManager::RegisterSystems()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Groups.size() > 0);
		auto& initializers = SystemInitializer::GetInitializers();

		struct SystemEntry
		{
			SystemId Id = UINT32_MAX;
			SystemConfig Config;
		};

		std::vector<SystemEntry> addedSystems;
		addedSystems.reserve(initializers.size());

		for (SystemInitializer* initializer : initializers)
		{
			FLARE_CORE_ASSERT(m_Registry.IsSystemIdValid(initializer->GetId()));

			SystemEntry& entry = addedSystems.emplace_back();
			entry.Id = initializer->GetId();

			SystemData& systemData = m_Systems.emplace_back();
			systemData.Id = initializer->GetId();
			systemData.SystemInstance = initializer->CreateSystem();

			m_Systems[initializer->GetId()].SystemInstance->OnConfig(m_World, entry.Config);

			FLARE_CORE_ASSERT(IsGroupIdValid(entry.Config.Group));

			AddSystemToGroup(initializer->GetId(), entry.Config.Group);
		}

		for (const SystemEntry& entry : addedSystems)
		{
			const auto& executionOrder = entry.Config.GetExecutionOrder();
			for (auto& order : executionOrder)
			{
				FLARE_CORE_ASSERT(m_Registry.IsSystemIdValid(order.ItemIndex));
			}

			AddSystemExecutionSettings(entry.Id, &entry.Config.GetExecutionOrder());
		}
	}

	void SystemsManager::SetDefaultSystemsGroup(SystemGroupId groupId)
	{
		FLARE_CORE_ASSERT(IsGroupIdValid(groupId));
		m_DefaultSystemGroupId = groupId;
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
		FLARE_PROFILE_FUNCTION();
		SystemData& data = m_Systems[system];

		if (executionOrder == nullptr || executionOrder != nullptr && executionOrder->size() == 0)
			m_Groups[data.GroupId].Graph.AddExecutionSettings();
		else
		{
			std::vector<ExecutionOrder> order = *executionOrder;
			for (auto& i : order)
			{
				FLARE_CORE_ASSERT(i.ItemIndex < (SystemId)m_Systems.size());
				i.ItemIndex = m_Systems[i.ItemIndex].IndexInGroup;
			}

			m_Groups[data.GroupId].Graph.AddExecutionSettings(std::move(order));
		}
	}

	void SystemsManager::ExecuteGroup(SystemGroupId id)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(id < (SystemGroupId)m_Groups.size());

		SystemGroup& group = m_Groups[id];
		for (size_t i : group.Graph.GetExecutionOrder())
		{
			const SystemData& data = m_Systems[group.SystemIndices[i]];

			SystemExecutionContext context{};
			context.Commands = &m_CommandBuffer;

			FLARE_CORE_ASSERT(data.SystemInstance != nullptr);

			data.SystemInstance->OnUpdate(m_World, context);
			m_CommandBuffer.Execute();
		}
	}

	bool SystemsManager::IsGroupIdValid(SystemGroupId id) const
	{
		return id < (SystemGroupId)m_Groups.size();
	}

	void SystemsManager::RebuildExecutionGraphs()
	{
		FLARE_PROFILE_FUNCTION();
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
