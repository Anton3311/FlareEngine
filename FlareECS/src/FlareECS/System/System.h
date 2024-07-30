#pragma once

#include "FlareECS/System/SystemData.h"

namespace Flare
{
	class World;
	struct SystemConfig
	{
		SystemConfig(SystemData& systemData)
			: m_Data(systemData) {}

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
	private:
		SystemData& m_Data;
	};

	class System
	{
	public:
		virtual ~System() {}

		virtual void OnConfig(World& world, SystemConfig& config) = 0;
		virtual void OnUpdate(World& world, SystemExecutionContext& context) = 0;
	};
}