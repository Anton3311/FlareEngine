#include "PCH.h"

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

		constexpr float SPACING_BETWEEN_LAYERS = 60.0f;
		constexpr float SPACING_BETWEEN_NODES = 30.0f;

		if (!m_IsVisible)
			return;

		const DependecyGraph& dependencyGraph = m_RenderGraph.GetDependencyGraph();
		if (dependencyGraph.GetMaxDependencyLayer() == 0)
			return;

		if (ImGui::Begin("Render Graph Visualizer", &m_IsVisible, ImGuiWindowFlags_NoMove))
		{
			const ImGuiStyle& style = ImGui::GetStyle();
			const ImU32 textColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
			const ImU32 lineColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

			if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
			{
				m_Offset = ImGui::GetMousePos() - m_MoveStartPosition;
			}
			else if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
			{
				m_MoveStartPosition = ImGui::GetMousePos() - m_Offset;
			}
			
			float graphOffset = 400.0f;

			m_LayerOffsets.resize(dependencyGraph.GetMaxDependencyLayer() + 1);
			m_NodePositions.resize(dependencyGraph.GetGraphNodes().size());

			m_NodePositions.assign(m_NodePositions.size(), glm::vec2(0.0f, 0.0f));
			m_LayerOffsets.assign(m_LayerOffsets.size(), graphOffset);

			const auto& nodes = dependencyGraph.GetGraphNodes();
			float textHeight = ImGui::GetFontSize();

			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImDrawList* drawList = window->DrawList;

			for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
			{
				const auto& node = nodes[nodeIndex];
				const char* name = node.PassNode->Specifications.GetDebugName().c_str();
				ImVec2 textSize = ImGui::CalcTextSize(name);

				ImVec2 textPosition(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
				ImVec2 nodeNameTextCenter = ImVec2(m_LayerOffsets[node.DependencyLayer], node.DependencyLayer * SPACING_BETWEEN_LAYERS);
				nodeNameTextCenter += m_Offset;

				m_LayerOffsets[node.DependencyLayer] += textSize.x + SPACING_BETWEEN_NODES;
				m_NodePositions[nodeIndex].x = nodeNameTextCenter.x + textSize.x / 2.0f;
				m_NodePositions[nodeIndex].y = nodeNameTextCenter.y + textSize.y / 2.0f;

				drawList->AddText(textPosition + nodeNameTextCenter, textColor, name);

				for (size_t dependencyIndex : node.Dependecies)
				{
					glm::vec2 dependencyPosition = m_NodePositions[dependencyIndex];

					ImVec2 start = ImVec2(dependencyPosition.x, dependencyPosition.y + textHeight / 2.0f);
					ImVec2 end = ImVec2(nodeNameTextCenter.x, nodeNameTextCenter.y - textHeight / 2.0f) + textSize / 2.0f;

					start += window->DC.CursorPos;
					end += window->DC.CursorPos;

					drawList->AddLine(start, end, lineColor);
				}
			}

			ImVec2 position = window->DC.CursorPos + style.FramePadding;
			for (size_t nodeIndex : dependencyGraph.GetExecutionOrder())
			{
				drawList->AddText(position, textColor, nodes[nodeIndex].PassNode->Specifications.GetDebugName().c_str());

				position.y += textHeight * 1.5f;
			}

			ImGui::End();
		}
	}
}
