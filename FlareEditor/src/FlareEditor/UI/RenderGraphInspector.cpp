#include "RenderGraphInspector.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
	RenderGraphInspector::RenderGraphInspector(const RenderGraph& renderGraph)
		: m_RenderGraph(renderGraph) {}

	void RenderGraphInspector::OnRenderImGui()
	{
		FLARE_PROFILE_FUNCTION();

		if (!m_IsVisible)
			return;

		const DependecyGraph& dependecyGraph = m_RenderGraph.GetDependecyGraph();
		if (dependecyGraph.GetMaxDependencyLayer() == 0)
			return;

		if (ImGui::Begin("Render Graph Visualizer", &m_IsVisible))
		{
			float graphOffset = 400.0f;

			m_LayerOffsets.resize(dependecyGraph.GetMaxDependencyLayer() + 1);
			m_NodePositions.resize(dependecyGraph.GetGraphNodes().size());

			m_NodePositions.assign(m_NodePositions.size(), glm::vec2(0.0f, 0.0f));
			m_LayerOffsets.assign(m_LayerOffsets.size(), graphOffset);

			const auto& nodes = dependecyGraph.GetGraphNodes();
			float textHeight = ImGui::GetFontSize();

			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImDrawList* drawList = window->DrawList;

			for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
			{
				const auto& node = nodes[nodeIndex];
				const char* name = node.PassNode->Specifications.GetDebugName().c_str();
				ImVec2 textSize = ImGui::CalcTextSize(name);

				const ImVec2 textPosition(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
				ImVec2 cursorPosition = ImVec2(m_LayerOffsets[node.DependencyLayer], node.DependencyLayer * textHeight * 5.0f);

				m_LayerOffsets[node.DependencyLayer] += textSize.x + 100.0f;
				m_NodePositions[nodeIndex].x = cursorPosition.x + textSize.x / 2.0f;
				m_NodePositions[nodeIndex].y = cursorPosition.y + textSize.y / 2.0f;

				drawList->AddText(textPosition + cursorPosition, UINT32_MAX, name);

				for (size_t dependecyIndex : node.Dependecies)
				{
					glm::vec2 dependecyPosition = m_NodePositions[dependecyIndex];

					ImVec2 start = ImVec2(dependecyPosition.x, dependecyPosition.y + textHeight / 2.0f);
					ImVec2 end = ImVec2(cursorPosition.x, cursorPosition.y - textHeight / 2.0f) + textSize / 2.0f;

					start += window->DC.CursorPos;
					end += window->DC.CursorPos;

					drawList->AddLine(start, end, UINT32_MAX);
				}
			}

			const ImGuiStyle& style = ImGui::GetStyle();

			ImVec2 position = window->DC.CursorPos + style.FramePadding;
			for (size_t nodeIndex : dependecyGraph.GetExecutionOrder())
			{
				drawList->AddText(position, UINT32_MAX, nodes[nodeIndex].PassNode->Specifications.GetDebugName().c_str());

				position.y += textHeight * 1.5f;
			}

			ImGui::End();
		}
	}
}
