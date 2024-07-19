#pragma once

#include "FlareECS/System/SystemData.h"

namespace Flare
{
	class World;
	struct SystemConfig
	{
		SystemGroupId Group;

		template<typename T>
		void ExecuteAfter()
		{
			static_assert(std::is_base_of_v<System, T>, "T must be System type");
			SystemId id = T::_SystemInitializer.GetId();
			FLARE_CORE_ASSERT(id != UINT32_MAX);
			m_ExecutionOrder.push_back(ExecutionOrder::After(id));
		}

		template<typename T>
		void ExecuteBefore()
		{
			static_assert(std::is_base_of_v<System, T>, "T must be System type");
			SystemId id = T::_SystemInitializer.GetId();
			FLARE_CORE_ASSERT(id != UINT32_MAX);
			m_ExecutionOrder.push_back(ExecutionOrder::Before(id));
		}

		constexpr const std::vector<ExecutionOrder>& GetExecutionOrder() const { return m_ExecutionOrder; }
	private:
		std::vector<ExecutionOrder> m_ExecutionOrder;
	};

	class System
	{
	public:
		virtual ~System() {}

		virtual void OnConfig(World& world, SystemConfig& config) = 0;
		virtual void OnUpdate(World& world, SystemExecutionContext& context) = 0;
	};
}