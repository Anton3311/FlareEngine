#include "RenderGraphVisualizer.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
	void RenderGraphVisualizer::OnRenderImGui(const RenderGraph& renderGraph)
	{
		FLARE_PROFILE_FUNCTION();
		const DependecyGraph& dependecyGraph = renderGraph.GetDependecyGraph();

		if (dependecyGraph.GetMaxDependencyLayer() == 0)
			return;

		float graphOffset = 400.0f;

		std::vector<float> offsets(dependecyGraph.GetMaxDependencyLayer() + 1, graphOffset);
		std::vector<ImVec2> positions(dependecyGraph.GetNodes().size(), ImVec2(0.0f, 0.0f));

		const auto& nodes = dependecyGraph.GetNodes();
		float textHeight = ImGui::GetFontSize();

		if (ImGui::Begin("Render Graph Visualizer"))
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImDrawList* drawList = window->DrawList;

			for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
			{
				const auto& node = nodes[nodeIndex];
				const char* name = node.PassNode->Specifications.GetDebugName().c_str();
				ImVec2 textSize = ImGui::CalcTextSize(name);

				const ImVec2 textPosition(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
				ImVec2 cursorPosition = ImVec2(offsets[node.DependencyLayer], node.DependencyLayer * textHeight * 5.0f);

				offsets[node.DependencyLayer] += textSize.x + 100.0f;
				positions[nodeIndex] = cursorPosition + textSize / 2.0f;

				drawList->AddText(textPosition + cursorPosition, UINT32_MAX, name);

				for (size_t dependecyIndex : node.Dependecies)
				{
					ImVec2 dependecyPosition = positions[dependecyIndex];

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
