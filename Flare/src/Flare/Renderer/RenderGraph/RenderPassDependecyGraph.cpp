#include "RenderPassDependecyGraph.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include <stack>
#include <deque>

namespace Flare
{
	struct AdjacencyMatrix
	{
	public:
		AdjacencyMatrix(uint32_t width, uint32_t height)
			: m_Width(width), m_Height(height)
		{
			m_Elements = new bool[width * height];
			std::memset(m_Elements, 0, width * height);
		}

		~AdjacencyMatrix()
		{
			delete[] m_Elements;
		}

		AdjacencyMatrix(const AdjacencyMatrix& other) noexcept
		{
			m_Width = other.m_Width;
			m_Height = other.m_Height;
			m_Elements = new bool[m_Width * m_Height];

			std::memcpy(m_Elements, other.m_Elements, m_Width * m_Height);
		}

		AdjacencyMatrix& operator=(const AdjacencyMatrix& other) noexcept
		{
			delete[] m_Elements;

			m_Width = other.m_Width;
			m_Height = other.m_Height;
			m_Elements = new bool[m_Width * m_Height];

			std::memcpy(m_Elements, other.m_Elements, m_Width * m_Height);
			return *this;
		}

		AdjacencyMatrix& operator=(AdjacencyMatrix&& other) noexcept
		{
			other.m_Width = 0;
			other.m_Height = 0;
			other.m_Elements = nullptr;

			m_Width = other.m_Width;
			m_Height = other.m_Height;
			m_Elements = other.m_Elements;
		}

		inline glm::uvec2 GetSize() const
		{
			return glm::uvec2(m_Width, m_Height);
		}

		inline void Set(uint32_t x, uint32_t y, bool value)
		{
			m_Elements[y * m_Width + x] = value;
		}

		inline bool Get(uint32_t x, uint32_t y) const
		{
			return m_Elements[y * m_Width + x];
		}

		AdjacencyMatrix operator*(const AdjacencyMatrix& b) const
		{
			FLARE_CORE_ASSERT(GetSize() == b.GetSize());
			FLARE_CORE_ASSERT(GetSize().x == GetSize().y);

			AdjacencyMatrix output(m_Width, m_Height);

			uint32_t size = GetSize().x;

			for (uint32_t y = 0; y < size; y++)
			{
				for (uint32_t x = 0; x < size; x++)
				{
					bool result = false;
					for (uint32_t i = 0; i < size; i++)
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

		AdjacencyMatrix& operator|=(const AdjacencyMatrix& other)
		{
			FLARE_CORE_ASSERT(GetSize() == other.GetSize());

			for (uint32_t y = 0; y < m_Height; y++)
			{
				for (uint32_t x = 0; x < m_Width; x++)
				{
					Set(x, y, Get(x, y) || other.Get(x, y));
				}
			}

			return *this;
		}

		AdjacencyMatrix NotAnd(const AdjacencyMatrix& other) const
		{
			FLARE_CORE_ASSERT(GetSize() == other.GetSize());
			AdjacencyMatrix output(m_Width, m_Height);

			for (uint32_t y = 0; y < m_Height; y++)
			{
				for (uint32_t x = 0; x < m_Width; x++)
				{
					output.Set(x, y, Get(x, y) && !other.Get(x, y));
				}
			}

			return output;
		}
	private:
		uint32_t m_Width = 0;
		uint32_t m_Height = 0;
		bool* m_Elements = nullptr;
	};

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

		AdjacencyMatrix adjacencyMatrix((uint32_t)m_Graph.size(), (uint32_t)m_Graph.size());

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

		for (uint32_t k = 0; k < adjacencyMatrix.GetSize().x; k++)
		{
			for (uint32_t y = 0; y < adjacencyMatrix.GetSize().y; y++)
			{
				for (uint32_t x = 0; x < adjacencyMatrix.GetSize().x; x++)
				{
					transitiveClosure.Set(x, y, transitiveClosure.Get(x, y) || transitiveClosure.Get(x, k) && transitiveClosure.Get(k, y));
				}
			}
		}

		AdjacencyMatrix ab = adjacencyMatrix * transitiveClosure;

		auto printMatrix = [](const AdjacencyMatrix& matrix)
			{
				for (uint32_t y = 0; y < matrix.GetSize().x; y++)
				{
					std::string line = "";
					for (uint32_t x = 0; x < matrix.GetSize().x; x++)
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
		for (uint32_t y = 0; y < adjacencyMatrix.GetSize().y; y++)
		{
			for (uint32_t x = 0; x < adjacencyMatrix.GetSize().x; x++)
			{
				if (transitiveClosure.Get(x, y))
				{
					FLARE_CORE_WARN("{} -> {}", x + 1, y + 1);
				}
			}
		}

		FLARE_CORE_INFO("Transitive Reduction:");
		for (uint32_t y = 0; y < adjacencyMatrix.GetSize().y; y++)
		{
			for (uint32_t x = 0; x < adjacencyMatrix.GetSize().x; x++)
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

	bool RenderPassDependecyGraph::IsReachable(GraphNode* start, GraphNode* target)
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<bool> visited(m_Graph.size(), false);
		return IsReachable(start, target, visited);
	}

	bool RenderPassDependecyGraph::IsReachable(GraphNode* start, GraphNode* target, std::vector<bool>& visited)
	{
		FLARE_PROFILE_FUNCTION();
		
		if (visited[start - m_Graph.data()])
			return false;

		visited[start - m_Graph.data()] = true;

		for (GraphNode* child : start->Children)
		{
			if (child == target)
				return true;

			if (!visited[child - m_Graph.data()])
			{
				if (IsReachable(child, target, visited))
					return true;
			}
		}

		return false;
	}

	std::vector<RenderPassDependecyGraph::GraphNode*> RenderPassDependecyGraph::CollectAllDependecies(GraphNode* node)
	{
		std::vector<GraphNode*> nodes;
		std::vector<bool> visited(m_Graph.size(), false);
		std::stack<GraphNode*> stack;

		stack.push(node);

		while (stack.size() > 0)
		{
			GraphNode* currentNode = stack.top();
			stack.pop();

			visited[currentNode - m_Graph.data()] = true;

			for (GraphNode* dependecy : currentNode->Dependecies)
			{
				if (!visited[dependecy - m_Graph.data()])
				{
					nodes.push_back(dependecy);
					stack.push(dependecy);
				}
			}
		}

		return nodes;
	}
}
