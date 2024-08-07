#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace Flare
{
	class RenderGraph;
	class RenderGraphInspector
	{
	public:
		RenderGraphInspector(const RenderGraph& renderGraph);

		void OnRenderImGui();

		inline void SetVisible(bool visible) { m_IsVisible = visible; }
	private:
		const RenderGraph& m_RenderGraph;

		bool m_IsVisible = false;

		std::vector<glm::vec2> m_NodePositions;
		std::vector<float> m_LayerOffsets;
	};
}