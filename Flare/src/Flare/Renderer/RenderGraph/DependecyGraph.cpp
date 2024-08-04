#include "DependecyGraph.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include <stack>
#include <deque>

namespace Flare
{
	AdjacencyMatrix::AdjacencyMatrix(uint32_t size)
		: m_Size(size)
	{
		FLARE_PROFILE_FUNCTION();
		m_Elements = new bool[m_Size * m_Size];
		std::memset(m_Elements, 0, m_Size * m_Size);
	}

	AdjacencyMatrix::~AdjacencyMatrix()
	{
		delete[] m_Elements;
	}

	AdjacencyMatrix::AdjacencyMatrix(const AdjacencyMatrix& other)
	{
		FLARE_PROFILE_FUNCTION();
		m_Size = other.m_Size;
		m_Elements = new bool[m_Size * m_Size];

		std::memcpy(m_Elements, other.m_Elements, m_Size * m_Size);
	}

	AdjacencyMatrix::AdjacencyMatrix(AdjacencyMatrix&& other) noexcept
	{
		m_Size = other.m_Size;
		m_Elements = other.m_Elements;

		other.m_Size = 0;
		other.m_Elements = nullptr;
	}

	AdjacencyMatrix& AdjacencyMatrix::operator=(const AdjacencyMatrix& other)
	{
		FLARE_PROFILE_FUNCTION();
		delete[] m_Elements;

		m_Size = other.m_Size;
		m_Elements = new bool[m_Size * m_Size];

		std::memcpy(m_Elements, other.m_Elements, m_Size * m_Size);
		return *this;
	}

	AdjacencyMatrix& AdjacencyMatrix::operator=(AdjacencyMatrix&& other) noexcept
	{
		m_Size = other.m_Size;
		m_Elements = other.m_Elements;

		other.m_Size = 0;
		other.m_Elements = nullptr;

		return *this;
	}

	AdjacencyMatrix AdjacencyMatrix::operator*(const AdjacencyMatrix& b) const
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(GetSize() == b.GetSize());

		AdjacencyMatrix output(m_Size);

		for (uint32_t y = 0; y < m_Size; y++)
		{
			for (uint32_t x = 0; x < m_Size; x++)
			{
				bool result = false;
				for (uint32_t i = 0; i < m_Size; i++)
				{
					bool aValue = Get(i, y);
					bool bValue = b.Get(x, i);
					result |= aValue && bValue;

				}

				output.Set(x, y, result);
			}
		}

		return output;
	}

	AdjacencyMatrix& AdjacencyMatrix::operator|=(const AdjacencyMatrix& other)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(GetSize() == other.GetSize());

		for (uint32_t y = 0; y < m_Size; y++)
		{
			for (uint32_t x = 0; x < m_Size; x++)
			{
				Set(x, y, Get(x, y) || other.Get(x, y));
			}
		}

		return *this;
	}

	AdjacencyMatrix AdjacencyMatrix::NotAnd(const AdjacencyMatrix& other) const
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(GetSize() == other.GetSize());
		AdjacencyMatrix output(m_Size);

		for (uint32_t y = 0; y < m_Size; y++)
		{
			for (uint32_t x = 0; x < m_Size; x++)
			{
				output.Set(x, y, Get(x, y) && !other.Get(x, y));
			}
		}

		return output;
	}

	static void PrintMatrix(const AdjacencyMatrix& matrix)
	{
		for (uint32_t y = 0; y < matrix.GetSize(); y++)
		{
			std::string line = "";
			for (uint32_t x = 0; x < matrix.GetSize(); x++)
			{
				if (matrix.Get(x, y))
					line += "1 ";
				else
					line += "0 ";
			}

			FLARE_CORE_TRACE(line);
		}
	}

	//
	// RenderPassDependecyGraph
	//

	DependecyGraph::DependecyGraph() {}

	DependecyGraph::DependecyGraph(Span<const RenderPassNode> nodes)
		: m_Nodes(nodes)
	{
		FLARE_PROFILE_FUNCTION();
		m_Graph.resize(m_Nodes.GetSize());

		for (size_t i = 0; i < m_Nodes.GetSize(); i++)
		{
			m_Graph[i].PassNode = &m_Nodes[i];
		}
	}

	void DependecyGraph::Build()
	{
		FLARE_PROFILE_FUNCTION();

		AdjacencyMatrix adjacencyMatrix((uint32_t)m_Graph.size());

		GenerateAdjacencyMatrix(adjacencyMatrix);

		AdjacencyMatrix transitiveClosure = adjacencyMatrix;
		GenerateTransitiveClosure(transitiveClosure);

		// Generate transitive reduction based on the transitive closure
		// https://en.wikipedia.org/wiki/Transitive_reduction#Computing_the_reduction_using_the_closure

		AdjacencyMatrix AB = adjacencyMatrix * transitiveClosure;

		{
			FLARE_PROFILE_SCOPE("GenerateDependecies");
			for (uint32_t y = 0; y < adjacencyMatrix.GetSize(); y++)
			{
				for (uint32_t x = 0; x < adjacencyMatrix.GetSize(); x++)
				{
					if (adjacencyMatrix.Get(x, y) && !AB.Get(x, y))
					{
						m_Graph[x].Dependecies.insert(&m_Graph[y]);
						m_Graph[y].Children.insert(&m_Graph[x]);
					}
				}
			}
		}

		for (GraphNode& node : m_Graph)
		{
			if (node.Dependecies.size() == 0)
			{
				DetermineDependencyLayers(&node);
			}
		}
	}

	void DependecyGraph::GenerateAdjacencyMatrix(AdjacencyMatrix& adjacencyMatrix)
	{
		FLARE_PROFILE_FUNCTION();

		for (GraphNode& node : m_Graph)
		{
			for (const auto& output : node.PassNode->Specifications.GetOutputs())
			{
				auto it = m_Writers.find(output.AttachmentTexture);
				if (it != m_Writers.end())
				{
					uint32_t nodeIndex = (uint32_t)(&node - m_Graph.data());

					for (GraphNode* dependecy : it->second)
					{
						adjacencyMatrix.Set(nodeIndex, (uint32_t)(dependecy - m_Graph.data()), true);
					}
				}
			}

			for (const auto& input : node.PassNode->Specifications.GetInputs())
			{
				auto it = m_Writers.find(input.InputTexture);
				if (it != m_Writers.end())
				{
					uint32_t nodeIndex = (uint32_t)(&node - m_Graph.data());

					for (GraphNode* dependecy : it->second)
					{
						adjacencyMatrix.Set(nodeIndex, (uint32_t)(dependecy - m_Graph.data()), true);
					}
				}
			}

			for (const auto& output : node.PassNode->Specifications.GetOutputs())
			{
				auto it = m_Writers.find(output.AttachmentTexture);
				if (it != m_Writers.end())
				{
					it->second.push_back(&node);
				}
				else
				{
					m_Writers[output.AttachmentTexture].push_back(&node);
				}
			}
		}
	}

	void DependecyGraph::GenerateTransitiveClosure(AdjacencyMatrix& matrix)
	{
		FLARE_PROFILE_FUNCTION();

		for (uint32_t k = 0; k < matrix.GetSize(); k++)
		{
			for (uint32_t y = 0; y < matrix.GetSize(); y++)
			{
				for (uint32_t x = 0; x < matrix.GetSize(); x++)
				{
					matrix.Set(x, y, matrix.Get(x, y) || matrix.Get(x, k) && matrix.Get(k, y));
				}
			}
		}
	}

	void DependecyGraph::DetermineDependencyLayers(GraphNode* start)
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<bool> visited(m_Graph.size(), false);
		std::deque<GraphNode*> queue;

		queue.push_back(start);

		start->DependencyLayer = 0;

		while (queue.size() > 0)
		{
			GraphNode* node = queue.front();
			queue.pop_front();

			if (visited[node - m_Graph.data()])
				continue;

			visited[node - m_Graph.data()] = true;

			for (GraphNode* child : node->Children)
			{
				uint32_t layer = node->DependencyLayer + 1;

				if (child->DependencyLayer == UINT32_MAX)
					child->DependencyLayer = layer;
				else
					child->DependencyLayer = glm::max(child->DependencyLayer, layer);

				m_MaxDependencyLayer = glm::max(m_MaxDependencyLayer, child->DependencyLayer);

				queue.push_back(child);
			}
		}
	}
}
