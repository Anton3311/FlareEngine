#pragma once

#include "FlareECS/Query/Query.h"

#include "FlareECS/System/System.h"
#include "FlareECS/System/SystemData.h"
#include "FlareECS/System/SystemsRegistry.h"
#include "FlareECS/System/ExecutionGraph/ExecutionGraph.h"

#include "FlareECS/Commands/CommandBuffer.h"

#include <functional>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Flare
{
	struct SystemGroup
	{
		SystemGroupId Id = INVALID_SYSTEM_GROUP_ID;
		std::string Name;
 
		std::vector<uint32_t> SystemIndices;

		bool ExecutionGraphIsDirty = false;

		std::vector<SystemId> ExecutionOrder;
	};

	class System;
	class World;
	class FLAREECS_API SystemsManager : public SystemsRegisteringHandler
	{
	public:
		SystemsManager(World& world, SystemsRegistry& registry);
		~SystemsManager();

		SystemGroupId CreateGroup(std::string_view name);
		std::optional<SystemGroupId> FindGroup(std::string_view name) const;

		void Clear();
		void ClearSystems();

		void RegisterSystems();

		void SetDefaultSystemsGroup(SystemGroupId groupId);

		void AddSystemToGroup(SystemId system, SystemGroupId group);

		void ExecuteGroup(SystemGroupId id);

		template<typename T>
		bool IsSystemEnabled() const
		{
			static_assert(std::is_base_of_v<System, T>);
			SystemId id = T::_SystemInitializer.GetId();

			FLARE_CORE_ASSERT(id < (uint32_t)m_Systems.size());

			return m_Systems[id].Enabled;
		}

		template<typename T>
		void SetSystemEnabled(bool enabled)
		{
			static_assert(std::is_base_of_v<System, T>);
			SystemId id = T::_SystemInitializer.GetId();

			FLARE_CORE_ASSERT(id < (uint32_t)m_Systems.size());

			SystemData& systemData = m_Systems[id];
			if (systemData.Enabled != enabled)
			{
				systemData.Enabled = enabled;
				FLARE_CORE_ASSERT(IsGroupIdValid(systemData.GroupId));

				m_Groups[systemData.GroupId].ExecutionGraphIsDirty = true;
			}
		}

		template<typename T>
		void ExecuteSystem()
		{
			static_assert(std::is_base_of_v<System, T>);
			SystemId id = T::_SystemInitializer.GetId();

			if (id < (uint32_t)m_Systems.size())
			{
				SystemExecutionContext context;
				context.Commands = &m_CommandBuffer;
				m_Systems[id].SystemInstance->OnUpdate(m_World, context);

				m_CommandBuffer.Execute();
			}
		}

		bool IsGroupIdValid(SystemGroupId id) const;
		void RebuildExecutionGraphs();

		const SystemsRegistry& GetSystemsRegistry() const { return m_Registry; }
		
		inline const std::vector<SystemGroup>& GetGroups() const { return m_Groups; }
		inline const std::vector<SystemData>& GetSystems() const { return m_Systems; }

		void OnUnregisterSystems() override;
		void OnRegisterSystems() override;
	private:
		World& m_World;
		SystemsRegistry& m_Registry;

		EntitiesCommandBuffer m_CommandBuffer;

		SystemGroupId m_DefaultSystemGroupId = 0;

		std::vector<SystemData> m_Systems;
		std::unordered_map<std::string, SystemGroupId> m_GroupNameToId;
		std::vector<SystemGroup> m_Groups;
	};
}