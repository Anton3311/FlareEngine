#pragma once

namespace Flare
{
	class RenderGraph;
	class RenderGraphVisualizer
	{
	public:
		static void OnRenderImGui(const RenderGraph& renderGraph);
	};
}