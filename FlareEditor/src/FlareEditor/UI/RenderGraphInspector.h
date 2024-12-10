#pragma once

#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <vector>
#include <glm/glm.hpp>
#include <optional>

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
		void RenderRenderGraph(ImRect viewportRect);
	private:
		const RenderGraph& m_RenderGraph;

		bool m_IsVisible = false;

		std::vector<glm::vec2> m_NodePositions;
		std::vector<float> m_LayerOffsets;

		ImVec2 m_MoveStartPosition = ImVec2(0.0f, 0.0f);
		ImVec2 m_Offset = ImVec2(0.0f, 0.0f);
	};
}