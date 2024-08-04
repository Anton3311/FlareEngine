#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/RenderGraph/RenderPassNode.h"
#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"

#include <unordered_set>

namespace Flare
{
	class FLARE_API RenderPassDependecyGraph
	{
	public:
		struct GraphNode
		{
			const RenderPassNode* PassNode = nullptr;
			std::unordered_set<GraphNode*> Children;
			std::unordered_set<GraphNode*> Dependecies;
		};

		RenderPassDependecyGraph(Span<const RenderPassNode> nodes);

		void Build();
	private:
		bool IsReachable(GraphNode* start, GraphNode* target);
		bool IsReachable(GraphNode* start, GraphNode* target, std::vector<bool>& visited);

		std::vector<GraphNode*> CollectAllDependecies(GraphNode* node);
	private:
		Span<const RenderPassNode> m_Nodes;

		std::vector<GraphNode> m_Graph;

		std::unordered_map<RenderGraphTextureId, std::vector<GraphNode*>> m_Writers;
	};
}
