#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/RenderGraph/RenderPassNode.h"
#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

#include <unordered_set>

namespace Flare
{
	// A sqaure boolean matrix
	struct FLARE_API AdjacencyMatrix
	{
	public:
		AdjacencyMatrix(uint32_t size);
		~AdjacencyMatrix();

		AdjacencyMatrix(const AdjacencyMatrix& other);
		AdjacencyMatrix(AdjacencyMatrix&& other) noexcept;

		AdjacencyMatrix& operator=(const AdjacencyMatrix& other);
		AdjacencyMatrix& operator=(AdjacencyMatrix&& other) noexcept;

		inline uint32_t GetSize() const { return m_Size; }
		inline void Set(uint32_t x, uint32_t y, bool value) { m_Elements[y * m_Size + x] = value; }
		inline bool Get(uint32_t x, uint32_t y) const { return m_Elements[y * m_Size + x]; }

		AdjacencyMatrix operator*(const AdjacencyMatrix& b) const;
		AdjacencyMatrix& operator|=(const AdjacencyMatrix& other);
		AdjacencyMatrix NotAnd(const AdjacencyMatrix& other) const;
	private:
		uint32_t m_Size = 0;
		bool* m_Elements = nullptr;
	};

	//
	// RenderPassDependecyGraph
	//

	class FLARE_API DependecyGraph
	{
	public:
		struct GraphNode
		{
			const RenderPassNode* PassNode = nullptr;
			std::unordered_set<GraphNode*> Children;
			std::unordered_set<GraphNode*> Dependecies;
		};

		DependecyGraph(Span<const RenderPassNode> nodes);

		void Build();
	private:
		void GenerateAdjacencyMatrix(AdjacencyMatrix& adjacencyMatrix);

		// Generates the transitive closure using Warshall algorithm
		void GenerateTransitiveClosure(AdjacencyMatrix& matrix);
	private:
		Span<const RenderPassNode> m_Nodes;

		std::vector<GraphNode> m_Graph;

		std::unordered_map<RenderGraphTextureId, std::vector<GraphNode*>> m_Writers;
	};
}
