#pragma once

#include "FlareECS/System/ExecutionGraph/ExecutionGraph.h"

#include <string>
#include <optional>

namespace Flare
{
	class EntitiesCommandBuffer;

	using SystemGroupId = uint32_t;
	using SystemId = uint32_t;

	struct SystemExecutionContext
	{
		EntitiesCommandBuffer* Commands = nullptr;
	};

	class System;
	struct SystemData
	{
	public:
		SystemData() = default;
	public:
		std::string Name;

		SystemExecutionContext ExecutionContext;
		System* SystemInstance = nullptr;

		SystemId Id = INT32_MAX;
		uint32_t IndexInGroup = UINT32_MAX;
		SystemGroupId GroupId = UINT32_MAX;
	};

	struct SystemGroup
	{
		SystemGroupId Id = UINT32_MAX;
		std::string Name;
 
		std::vector<uint32_t> SystemIndices;

		ExecutionGraph Graph;
	};
}