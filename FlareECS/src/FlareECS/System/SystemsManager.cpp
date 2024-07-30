#include "SystemsManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"
#include "FlareECS/System/SystemInitializer.h"
#include "FlareECS/System/SystemsRegistry.h"

namespace Flare
{
	SystemsManager::SystemsManager(World& world, SystemsRegistry& registry)
		: m_CommandBuffer(world), m_World(world), m_Registry(registry)
	{
		m_Registry.AddResigteringHandler(this);
	}

	SystemsManager::~SystemsManager()
	{
		m_Registry.RemoveRegisteringHandler(this);

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

	void SystemsManager::Clear()
	{
		FLARE_PROFILE_FUNCTION();

		m_Systems.clear();
		m_Groups.clear();
		m_GroupNameToId.clear();
	}

	void SystemsManager::ClearSystems()
	{
		FLARE_PROFILE_FUNCTION();
		m_Systems.clear();

		for (SystemGroup& group : m_Groups)
		{
			group.ExecutionOrder.clear();
			group.SystemIndices.clear();
		}
	}

	void SystemsManager::RegisterSystems()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Groups.size() > 0);
		auto& initializers = SystemInitializer::GetInitializers();

		for (SystemInitializer* initializer : initializers)
		{
			FLARE_CORE_ASSERT(m_Registry.IsSystemIdValid(initializer->GetId()));

			SystemData& systemData = m_Systems.emplace_back();
			systemData.Id = initializer->GetId();
			systemData.SystemInstance = initializer->CreateSystem();

			SystemConfig config(systemData);

			systemData.SystemInstance->OnConfig(m_World, config);

			if (!IsGroupIdValid(config.Group))
			{
				config.Group = m_DefaultSystemGroupId;
			}

			AddSystemToGroup(initializer->GetId(), config.Group);
		}

		for (SystemData& system : m_Systems)
		{
			for (SystemId dependentSystem : system.GetDependentSystems())
			{
				m_Systems[dependentSystem].AddDependecy(system.Id);
			}
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

	void SystemsManager::ExecuteGroup(SystemGroupId id)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(id < (SystemGroupId)m_Groups.size());

		SystemGroup& group = m_Groups[id];
		for (SystemId id : group.ExecutionOrder)
		{
			const SystemData& data = m_Systems[id];

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
			group.ExecutionOrder.clear();

			ExecutionGraph graph(
				Span<const SystemData>(m_Systems.data(), m_Systems.size()),
				Span<const SystemId>(group.SystemIndices.data(), group.SystemIndices.size()),
				group.ExecutionOrder);

			if (graph.RebuildGraph() == ExecutionGraph::BuildResult::CircularDependecy)
			{
				FLARE_CORE_ERROR("Failed to build an execution graph for '{0}' because of circular dependecy", group.Name);
				continue;
			}
		}
	}

	void SystemsManager::OnUnregisterSystems()
	{
		FLARE_PROFILE_FUNCTION();

		ClearSystems();
	}

	void SystemsManager::OnRegisterSystems()
	{
		FLARE_PROFILE_FUNCTION();
		RegisterSystems();
	}
}
