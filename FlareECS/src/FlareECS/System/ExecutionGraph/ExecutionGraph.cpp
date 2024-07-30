#include "ExecutionGraph.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"

#include <unordered_set>
#include <queue>
#include <stack>

namespace Flare
{
	ExecutionGraph::ExecutionGraph(Span<const SystemData> systems, Span<const SystemId> groupSystems, std::vector<SystemId>& outExecutionOrder)
		: m_Systems(systems), m_GroupSystems(groupSystems), m_OutExecutionOrder(outExecutionOrder)
	{
	}

	ExecutionGraph::BuildResult ExecutionGraph::RebuildGraph()
	{
		FLARE_PROFILE_FUNCTION();
#if 0
		m_Graph.resize(m_ExecutionSettings.size());
		for (size_t nodeIndex = 0; nodeIndex < m_Graph.size(); nodeIndex++)
		{
			GraphNode& node = m_Graph[nodeIndex];

			for (const ExecutionOrder& order : m_ExecutionSettings[nodeIndex])
			{
				FLARE_CORE_ASSERT(order.ItemIndex <= (uint32_t)m_Graph.size());
				switch (order.ExecutionOrder)
				{
				case ExecutionOrder::Order::After:
					node.Dependecies.push_back(order.ItemIndex);
					m_Graph[order.ItemIndex].Children.push_back((uint32_t)nodeIndex);
					break;
				case ExecutionOrder::Order::Before:
					m_Graph[order.ItemIndex].Dependecies.push_back((uint32_t)nodeIndex);
					node.Children.push_back(order.ItemIndex);
					break;
				default:
					FLARE_CORE_ASSERT(false, "Unreachable");
				}
			}
		}
#endif
		m_Visited.resize(m_Systems.GetSize(), VisitedFlag::None);

		if (CheckForCicularDependecies())
			return BuildResult::CircularDependecy;

		m_Visited.assign(m_Systems.GetSize(), VisitedFlag::None);
		m_OutExecutionOrder.reserve(m_Systems.GetSize());

		std::unordered_set<SystemId> unresolvedNodes;

		for (SystemId system : m_GroupSystems)
		{
			if (m_Systems[system].GetDependecies().GetSize() == 0)
				GenerateExecutionOrderList(system, unresolvedNodes);
			else
				unresolvedNodes.insert(system);
		}

		return BuildResult::Success;
	}

	void ExecutionGraph::GenerateExecutionOrderList(SystemId initialSystem, std::unordered_set<SystemId>& unresolvedNodes)
	{
		FLARE_PROFILE_FUNCTION();
		std::queue<SystemId> queue;
		queue.push(initialSystem);

		constexpr size_t MAX_ITER = 100;
		size_t iter = 0;

		std::vector<uint32_t> resolved;
		while (iter < MAX_ITER)
		{
			resolved.clear();
			for (uint32_t unres : unresolvedNodes)
			{
				if (!HasIncompleteDependecies(unres))
				{
					queue.push(unres);
					resolved.push_back(unres);
				}
			}

			for (uint32_t i : resolved)
				unresolvedNodes.erase(i);

			if (queue.size() == 0)
				break;

			SystemId systemId = queue.front();
			queue.pop();

			const SystemData& node = m_Systems[systemId];
			m_Visited[systemId] = VisitedFlag::Visited;

			m_OutExecutionOrder.push_back(systemId);

			for (SystemId childNode : node.GetDependentSystems())
			{
				if (unresolvedNodes.find(childNode) != unresolvedNodes.end())
					continue;

				if (!HasIncompleteDependecies(childNode))
					queue.push(childNode);
			}

			iter++;
		}

		FLARE_CORE_ASSERT(iter != MAX_ITER);
	}

	bool ExecutionGraph::CheckForCicularDependecies()
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Systems.GetSize() == 0)
			return false;

		for (SystemId system : m_GroupSystems)
		{
			if (m_Visited[system] == VisitedFlag::None)
			{
				if (CheckForCicularDependecies(system))
					return true;
			}
		}

		return false;
	}

	bool ExecutionGraph::CheckForCicularDependecies(SystemId node)
	{
		FLARE_PROFILE_FUNCTION();
		m_Visited[node] = VisitedFlag::Visited | VisitedFlag::InCurrentPath;

		for (SystemId dependecy : m_Systems[node].GetDependecies())
		{
			if (HAS_BIT(m_Visited[dependecy], VisitedFlag::Visited))
			{
				if (HAS_BIT(m_Visited[dependecy], VisitedFlag::InCurrentPath))
					return true;
			}
			else if (CheckForCicularDependecies(dependecy))
				return true;
		}

		m_Visited[node] = m_Visited[node] & ~VisitedFlag::InCurrentPath;
		return false;
	}

	bool ExecutionGraph::HasIncompleteDependecies(SystemId systemId)
	{
		FLARE_PROFILE_FUNCTION();
		for (SystemId depedency : m_Systems[systemId].GetDependecies())
		{
			FLARE_CORE_ASSERT(depedency < (uint32_t)m_Visited.size());
			if (m_Visited[depedency] == VisitedFlag::None)
				return true;
		}
		return false;
	}
}
