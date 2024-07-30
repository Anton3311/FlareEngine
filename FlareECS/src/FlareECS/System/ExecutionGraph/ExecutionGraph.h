#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include "FlareECS/System/SystemData.h"

#include <vector>
#include <unordered_set>
#include <stdint.h>

namespace Flare
{
	struct ExecutionOrder
	{
		enum class Order : uint8_t
		{
			Before,
			After,
		};

		inline static ExecutionOrder After(uint32_t index)
		{
			return { Order::After, index };
		}

		inline static ExecutionOrder Before(uint32_t index)
		{
			return { Order::Before, index };
		}

		Order ExecutionOrder;
		uint32_t ItemIndex;
	};

	FLARE_IMPL_ENUM_BITFIELD(ExecutionOrder::Order);

	class FLAREECS_API ExecutionGraph
	{
	public:
		struct GraphNode
		{
			std::vector<uint32_t> Dependecies;
			std::vector<uint32_t> Children;
		};

		enum class VisitedFlag : uint8_t
		{
			None = 0,
			Visited = 1,
			InCurrentPath = 2
		};

		enum class BuildResult
		{
			Success,
			CircularDependecy,
		};
	public:
		ExecutionGraph(Span<const SystemData> systems, Span<const SystemId> groupSystems, std::vector<SystemId>& outExecutionOrder);

		BuildResult RebuildGraph();
	private:
		void GenerateExecutionOrderList(SystemId initialSystem, std::unordered_set<SystemId>& unresolvedNodes);
		bool CheckForCicularDependecies();
		bool CheckForCicularDependecies(SystemId node);
		bool HasIncompleteDependecies(SystemId systemId);
	private:
		std::vector<SystemId>& m_OutExecutionOrder;
		Span<const SystemData> m_Systems;
		Span<const SystemId> m_GroupSystems;

		//std::vector<std::vector<ExecutionOrder>> m_ExecutionSettings;
		std::vector<VisitedFlag> m_Visited;
		//std::vector<GraphNode> m_Graph;
	};

	FLARE_IMPL_ENUM_BITFIELD(ExecutionGraph::VisitedFlag);
}