#pragma once

#include "FlareECS/Query/Query.h"

#include "FlareECS/System/System.h"
#include "FlareECS/System/SystemData.h"

#include "FlareECS/Commands/CommandBuffer.h"

#include <functional>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Flare
{
	class System;
	class World;
	class FLAREECS_API SystemsManager
	{
	public:
		SystemsManager(World& world);
		~SystemsManager();

		SystemGroupId CreateGroup(std::string_view name);
		std::optional<SystemGroupId> FindGroup(std::string_view name) const;

		SystemId RegisterSystem(std::string_view name, System* systemInstance);

		void RegisterSystems();

		void SetDefaultSystemsGroup(SystemGroupId groupId);

		void AddSystemToGroup(SystemId system, SystemGroupId group);
		void AddSystemExecutionSettings(SystemId system, const std::vector<ExecutionOrder>* executionOrder);

		void ExecuteGroup(SystemGroupId id);

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
			}
		}

		bool IsGroupIdValid(SystemGroupId id);
		void RebuildExecutionGraphs();
		
		const std::vector<SystemGroup>& GetGroups() const;
		std::vector<SystemGroup>& GetGroups();

		const std::vector<SystemData>& GetSystems() const;
	private:
		World& m_World;
		EntitiesCommandBuffer m_CommandBuffer;

		SystemGroupId m_DefaultSystemGroupId = 0;

		std::vector<SystemData> m_Systems;
		std::unordered_map<std::string, SystemGroupId> m_GroupNameToId;
		std::vector<SystemGroup> m_Groups;
	};
}