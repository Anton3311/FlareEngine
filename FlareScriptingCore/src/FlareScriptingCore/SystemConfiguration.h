#pragma once

#include "FlareECS/ComponentId.h"
#include "FlareECS/QueryFilters.h"

#include <vector>

namespace Flare::Internal
{
	class SystemQuery
	{
	public:
		SystemQuery(std::vector<ComponentId>* buffer)
			: m_Buffer(buffer) {}

		SystemQuery(const SystemQuery&) = delete;
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
		SystemConfiguration(std::vector<ComponentId>* queryOutputBuffer)
			: Query(queryOutputBuffer) {}

		SystemConfiguration(SystemConfiguration&) = delete;
		SystemConfiguration& operator=(const SystemConfiguration&) = delete;
	public:
		SystemQuery Query;
	};
}