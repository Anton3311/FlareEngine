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
						m_Graph[x].Dependecies.insert(y);
						m_Graph[y].Children.insert(x);
					}
				}
			}
		}

		for (size_t i = 0; i < m_Graph.size(); i++)
		{
			if (m_Graph[i].Dependecies.size() == 0)
			{
				DetermineDependencyLayers(i);
			}
		}

		GenerateExecutionOrder();
	}

	void DependecyGraph::GenerateAdjacencyMatrix(AdjacencyMatrix& adjacencyMatrix)
	{
		FLARE_PROFILE_FUNCTION();

		std::unordered_map<RenderGraphTextureId, std::vector<size_t>> writingPasses;

		auto createConnections = [&writingPasses, &adjacencyMatrix](RenderGraphTextureId textureId, size_t nodeIndex)
			{
				auto it = writingPasses.find(textureId);
				if (it != writingPasses.end())
				{
					for (size_t dependecyIndex : it->second)
					{
						adjacencyMatrix.Set((uint32_t)nodeIndex, (uint32_t)dependecyIndex, true);
					}
				}
			};

		for (size_t nodeIndex = 0; nodeIndex < m_Graph.size(); nodeIndex++)
		{
			const GraphNode& node = m_Graph[nodeIndex];
			RenderGraphPassType passType = node.PassNode->Specifications.GetType();

			switch (passType)
			{
			case RenderGraphPassType::Graphics:
			case RenderGraphPassType::Other:
				for (const auto& output : node.PassNode->Specifications.GetOutputs())
					createConnections(output.AttachmentTexture, nodeIndex);

				for (const auto& input : node.PassNode->Specifications.GetInputs())
					createConnections(input.InputTexture, nodeIndex);

				for (const auto& output : node.PassNode->Specifications.GetOutputs())
					writingPasses[output.AttachmentTexture].push_back(nodeIndex);
				break;
			case RenderGraphPassType::Compute:
				for (const auto& input : node.PassNode->Specifications.GetInputs())
					createConnections(input.InputTexture, nodeIndex);

				for (const auto& resource : node.PassNode->Specifications.GetGeneralTextureResources())
					createConnections(resource.TextureId, nodeIndex);

				for (const auto& resource : node.PassNode->Specifications.GetGeneralTextureResources())
				{
					if (HAS_BIT(resource.Access, ResourceAccess::Write))
						writingPasses[resource.TextureId].push_back(nodeIndex);
				}
				break;
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

	void DependecyGraph::DetermineDependencyLayers(size_t start)
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<bool> visited(m_Graph.size(), false);
		std::deque<size_t> queue;

		queue.push_back(start);

		m_Graph[start].DependencyLayer = 0;

		while (queue.size() > 0)
		{
			size_t nodeIndex = queue.front();
			queue.pop_front();

			if (visited[nodeIndex])
				continue;

			visited[nodeIndex] = true;

			const GraphNode& node = m_Graph[nodeIndex];
			for (size_t childIndex : node.Children)
			{
				GraphNode& child = m_Graph[childIndex];
				uint32_t layer = node.DependencyLayer + 1;

				if (child.DependencyLayer == UINT32_MAX)
					child.DependencyLayer = layer;
				else
					child.DependencyLayer = glm::max(child.DependencyLayer, layer);

				m_MaxDependencyLayer = glm::max(m_MaxDependencyLayer, child.DependencyLayer);

				queue.push_back(childIndex);
			}
		}
	}

	void DependecyGraph::GenerateExecutionOrder()
	{
		FLARE_PROFILE_FUNCTION();

		m_ExecutionOrder.reserve(m_Graph.size());
		std::vector<uint32_t> visited(m_Graph.size(), 0);

		std::deque<size_t> queue;

		for (size_t i = 0; i < m_Graph.size(); i++)
		{
			if (m_Graph[i].Dependecies.size() == 0)
				queue.push_back(i);
		}

		while (queue.size() > 0)
		{
			size_t nodeIndex = queue.front();
			queue.pop_front();

			if (visited[nodeIndex] != (uint32_t)m_Graph[nodeIndex].Dependecies.size())
			{
				queue.push_back(nodeIndex);
				continue;
			}

			m_Graph[nodeIndex].OrderIndex = (uint32_t)m_ExecutionOrder.size();
			m_ExecutionOrder.push_back(nodeIndex);

			for (size_t childIndex : m_Graph[nodeIndex].Children)
			{
				if (visited[childIndex] == 0)
					queue.push_back(childIndex);

				visited[childIndex]++;
			}
		}
	}
}
