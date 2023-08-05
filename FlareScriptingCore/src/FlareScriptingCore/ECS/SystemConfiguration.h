#pragma once

#include "FlareECS/ComponentId.h"
#include "FlareECS/QueryFilters.h"
#include "FlareECS/System.h"

#include <vector>

namespace Flare::Scripting
{
	class SystemConfiguration
	{
	public:
		SystemConfiguration(SystemGroupId defaultGroup)
			: Group(defaultGroup) {}

		SystemConfiguration(SystemConfiguration&) = delete;
		SystemConfiguration(SystemConfiguration&& other) noexcept
			: Group(other.Group), m_SystemExecutionOrder(std::move(other.m_SystemExecutionOrder)) {}
		SystemConfiguration& operator=(const SystemConfiguration&) = delete;

		inline const std::vector<ExecutionOrder>& GetExecutionOrder() const { return m_SystemExecutionOrder; }

		template<typename SystemT>
		constexpr void ExecuteAfter()
		{
			m_SystemExecutionOrder.push_back(ExecutionOrder::After((uint32_t) SystemT::System.Id));
		}

		template<typename SystemT>
		constexpr void ExecuteBefore()
		{
			m_SystemExecutionOrder.push_back(ExecutionOrder::Before((uint32_t) SystemT::System.Id));
		}
	public:
		SystemGroupId Group;
	private:
		std::vector<ExecutionOrder> m_SystemExecutionOrder;
	};
}