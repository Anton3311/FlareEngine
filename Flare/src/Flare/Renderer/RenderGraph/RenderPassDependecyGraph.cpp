#include "RenderPassDependecyGraph.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include <stack>
#include <deque>

namespace Flare
{
	AdjacencyMatrix::AdjacencyMatrix(uint32_t size)
		: m_Size(size)
	{
		m_Elements = new bool[m_Size * m_Size];
		std::memset(m_Elements, 0, m_Size * m_Size);
	}

	AdjacencyMatrix::~AdjacencyMatrix()
	{
		delete[] m_Elements;
	}

	AdjacencyMatrix::AdjacencyMatrix(const AdjacencyMatrix& other)
	{
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

	//
	// RenderPassDependecyGraph
	//

	RenderPassDependecyGraph::RenderPassDependecyGraph(Span<const RenderPassNode> nodes)
		: m_Nodes(nodes)
	{
		FLARE_PROFILE_FUNCTION();
		m_Graph.resize(m_Nodes.GetSize());

		for (size_t i = 0; i < m_Nodes.GetSize(); i++)
		{
			m_Graph[i].PassNode = &m_Nodes[i];
		}
	}

	void RenderPassDependecyGraph::Build()
	{
		FLARE_PROFILE_FUNCTION();

		AdjacencyMatrix adjacencyMatrix((uint32_t)m_Graph.size());

		for (GraphNode& node : m_Graph)
		{
			FLARE_CORE_INFO("Node: {}", node.PassNode->Specifications.GetDebugName());
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

		AdjacencyMatrix transitiveClosure = adjacencyMatrix;

		for (uint32_t k = 0; k < adjacencyMatrix.GetSize(); k++)
		{
			for (uint32_t y = 0; y < adjacencyMatrix.GetSize(); y++)
			{
				for (uint32_t x = 0; x < adjacencyMatrix.GetSize(); x++)
				{
					transitiveClosure.Set(x, y, transitiveClosure.Get(x, y) || transitiveClosure.Get(x, k) && transitiveClosure.Get(k, y));
				}
			}
		}

		AdjacencyMatrix ab = adjacencyMatrix * transitiveClosure;

		auto printMatrix = [](const AdjacencyMatrix& matrix)
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
			};

		FLARE_CORE_INFO("Matrix");
		printMatrix(adjacencyMatrix);
		FLARE_CORE_INFO("AB");
		printMatrix(ab);
		FLARE_CORE_INFO("Transitive Closure");
		printMatrix(transitiveClosure);

		FLARE_CORE_INFO("Transitive Closure 2:");
		for (uint32_t y = 0; y < adjacencyMatrix.GetSize(); y++)
		{
			for (uint32_t x = 0; x < adjacencyMatrix.GetSize(); x++)
			{
				if (transitiveClosure.Get(x, y))
				{
					FLARE_CORE_WARN("{} -> {}", x + 1, y + 1);
				}
			}
		}

		FLARE_CORE_INFO("Transitive Reduction:");
		for (uint32_t y = 0; y < adjacencyMatrix.GetSize(); y++)
		{
			for (uint32_t x = 0; x < adjacencyMatrix.GetSize(); x++)
			{
				if (adjacencyMatrix.Get(x, y) && !ab.Get(x, y))
				{
					FLARE_CORE_WARN("{} -> {}", x + 1, y + 1);
				}
			}
		}

		AdjacencyMatrix sdf = transitiveClosure.NotAnd(ab);
		FLARE_CORE_INFO("jslkdf");
		printMatrix(sdf);
		
		return;

		FLARE_CORE_WARN("");
		for (GraphNode& node : m_Graph)
		{
			FLARE_CORE_ERROR("Node: {}", node.PassNode->Specifications.GetDebugName());
			FLARE_CORE_INFO("Dependecies:");

			for (GraphNode* dependecy : node.Dependecies)
			{
				FLARE_CORE_TRACE("\t{}", dependecy->PassNode->Specifications.GetDebugName());
			}
		}
		FLARE_CORE_WARN("");
	}
}
