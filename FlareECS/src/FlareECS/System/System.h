#pragma once

#include "FlareECS/System/SystemData.h"

namespace Flare
{
	class World;
	class SystemsManager;
	struct SystemConfig
	{
		SystemConfig(SystemData& systemData, const SystemsManager& systemsManager)
			: m_Data(systemData), SystemsManager(systemsManager) {}

		template<typename T>
		void ExecuteAfter()
		{
			static_assert(std::is_base_of_v<System, T>, "T must be System type");
			SystemId id = T::_SystemInitializer.GetId();
			FLARE_CORE_ASSERT(id != INVALID_SYSTEM_ID);

			m_Data.AddDependecy(id);
		}

		template<typename T>
		void ExecuteBefore()
		{
			static_assert(std::is_base_of_v<System, T>, "T must be System type");
			SystemId id = T::_SystemInitializer.GetId();
			FLARE_CORE_ASSERT(id != INVALID_SYSTEM_ID);

			m_Data.AddDependentSystem(id);
		}
	public:
		SystemGroupId Group = INVALID_SYSTEM_GROUP_ID;
		const SystemsManager& SystemsManager;
	private:
		SystemData& m_Data;
	};

	class FLAREECS_API System
	{
	public:
		virtual ~System() = default;

		virtual void OnConfig(World& world, SystemConfig& config) = 0;
		virtual void OnUpdate(World& world, SystemExecutionContext& context) = 0;
	};
}