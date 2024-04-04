#pragma once

#include "FlareECS/Query/Query.h"
#include "FlareECS/System/ExecutionGraph/ExecutionGraph.h"

#include "FlareECS/Commands/CommandBuffer.h"

#include <functional>

namespace Flare
{
	using SystemGroupId = uint32_t;
	using SystemId = uint32_t;

	struct SystemExecutionContext
	{
		EntitiesCommandBuffer* Commands = nullptr;
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