#pragma once

#include "FlareECS/Query/Query.h"
#include "FlareECS/System.h"

#include "FlareECS/System/ExecutionGraph/ExecutionGraph.h"

#include <functional>

namespace Flare
{
	class SystemExecutionContext
	{
	public:
		SystemExecutionContext() = default;
		SystemExecutionContext(const Query& query)
			: m_Query(query) {}
	public:
		constexpr Query GetQuery() const { return m_Query; }
		constexpr void SetQuery(const Query& query) { m_Query = query; }
	private:
		Query m_Query;
	};

	using SystemEventFunction = std::function<void(SystemExecutionContext& context)>;

	struct SystemData
	{
		SystemData() = default;
		SystemData(const Query& query)
			: Id(UINT32_MAX),
			GroupId(UINT32_MAX),
			IndexInGroup(UINT32_MAX) {}

		std::string Name;

		SystemExecutionContext ExecutionContext;

		SystemEventFunction OnBeforeUpdate;
		SystemEventFunction OnAfterUpdate;
		SystemEventFunction OnUpdate;

		SystemId Id;
		uint32_t IndexInGroup;
		SystemGroupId GroupId;
	};

	class SystemsManager;
	struct SystemGroup
	{
		SystemGroupId Id;
		std::string Name;
 
		std::vector<uint32_t> SystemIndices;

		ExecutionGraph Graph;
	};
}