#include "ExecutionGraph.h"

#include "Flare/Core/Assert.h"

#include <unordered_set>
#include <queue>
#include <stack>

namespace Flare
{
	constexpr ExecutionGraph::VisitedFlag operator|(ExecutionGraph::VisitedFlag a, ExecutionGraph::VisitedFlag b)
	{
		return (ExecutionGraph::VisitedFlag)((uint8_t)a | (uint8_t)b);
	}

	constexpr ExecutionGraph::VisitedFlag operator&(ExecutionGraph::VisitedFlag a, ExecutionGraph::VisitedFlag b)
	{
		return (ExecutionGraph::VisitedFlag)((uint8_t)a & (uint8_t)b);
	}

	constexpr ExecutionGraph::VisitedFlag operator~(ExecutionGraph::VisitedFlag a)
	{
		return (ExecutionGraph::VisitedFlag)(~(uint8_t)a);
	}

	constexpr bool operator!=(ExecutionGraph::VisitedFlag a, int32_t b)
	{
		return (int32_t)a != b;
	}

	void ExecutionGraph::AddExecutionSettings()
	{
		m_ExecutionSettings.emplace_back();
	}

	void ExecutionGraph::AddExecutionSettings(const std::vector<ExecutionOrder> settings)
	{
		m_ExecutionSettings.push_back(std::move(settings));
	}

	ExecutionGraph::BuildResult ExecutionGraph::RebuildGraph()
	{
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

		m_Visited.resize(m_Graph.size(), VisitedFlag::None);

		if (CheckForCicularDependecies())
			return BuildResult::CircularDependecy;

		m_Visited.assign(m_Graph.size(), VisitedFlag::None);
		m_ExecutionOrder.reserve(m_Graph.size());

		std::unordered_set<uint32_t> unresolvedNodes;
		for (size_t i = 0; i < m_Graph.size(); i++)
		{
			if (m_Graph[i].Dependecies.size() == 0)
				GenerateExecutionOrderList((uint32_t)i, unresolvedNodes);
		}
	}

	void ExecutionGraph::GenerateExecutionOrderList(uint32_t initialNode, std::unordered_set<uint32_t>& unresolvedNodes)
	{
		std::queue<uint32_t> queue;
		queue.push(initialNode);

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

			uint32_t nodeIndex = queue.front();
			queue.pop();

			FLARE_CORE_ASSERT(nodeIndex < (uint32_t)m_Graph.size());

			const GraphNode& node = m_Graph[nodeIndex];
			m_Visited[nodeIndex] = VisitedFlag::Visited;

			m_ExecutionOrder.push_back(nodeIndex);

			for (uint32_t childNode : node.Children)
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
		if (m_Graph.size() == 0)
			return false;

		for (uint32_t i = 0; i < (uint32_t)m_Graph.size(); i++)
		{
			if (m_Visited[i] == VisitedFlag::None)
			{
				if (CheckForCicularDependecies(i))
					return true;
			}
		}

		return false;
	}

	bool ExecutionGraph::CheckForCicularDependecies(uint32_t node)
	{
		FLARE_CORE_ASSERT(node < (uint32_t)m_Graph.size());
		m_Visited[node] = VisitedFlag::Visited | VisitedFlag::InCurrentPath;

		for (uint32_t dependecy : m_Graph[node].Dependecies)
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

	bool ExecutionGraph::HasIncompleteDependecies(uint32_t index)
	{
		FLARE_CORE_ASSERT(index < (uint32_t)m_Graph.size());
		for (uint32_t depedency : m_Graph[index].Dependecies)
		{
			FLARE_CORE_ASSERT(depedency < (uint32_t)m_Visited.size());
			if (m_Visited[depedency] == VisitedFlag::None)
				return true;
		}
		return false;
	}
}
