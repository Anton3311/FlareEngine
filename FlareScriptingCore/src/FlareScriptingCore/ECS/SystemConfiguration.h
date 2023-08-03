#pragma once

#include "FlareECS/ComponentId.h"
#include "FlareECS/QueryFilters.h"
#include "FlareECS/System.h"

#include <vector>

namespace Flare::Scripting
{
	class SystemQuery
	{
	public:
		SystemQuery(std::vector<ComponentId>* buffer)
			: m_Buffer(buffer) {}

		SystemQuery(const SystemQuery&) = delete;
		SystemQuery(SystemQuery&& other) noexcept
			: m_Buffer(other.m_Buffer)
		{
			other.m_Buffer = nullptr;
		}

		SystemQuery& operator=(const SystemQuery&) = delete;
	public:
		template<typename ComponentT>
		void With()
		{
			m_Buffer->push_back(ComponentT::Info.Id);
		}

		template<typename ComponentT>
		void Without()
		{
			m_Buffer->push_back(ComponentId(
				ComponentT::Info.Id.GetIndex() | (uint32_t)QueryFilterType::Without,
				ComponentT::Info.Id.GetGeneration()));
		}
	private:
		std::vector<ComponentId>* m_Buffer;
	};

	class SystemConfiguration
	{
	public:
		SystemConfiguration(std::vector<ComponentId>* queryOutputBuffer, SystemGroupId defaultGroup)
			: Query(queryOutputBuffer), Group(defaultGroup) {}

		SystemConfiguration(SystemConfiguration&) = delete;
		SystemConfiguration(SystemConfiguration&& other) noexcept
			: Query(std::move(other.Query)), Group(other.Group), m_SystemExecutionOrder(std::move(other.m_SystemExecutionOrder)) {}
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
		SystemQuery Query;
		SystemGroupId Group;
	private:
		std::vector<ExecutionOrder> m_SystemExecutionOrder;
	};
}