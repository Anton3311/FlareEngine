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

		std::vector<float> offsets(dependecyGraph.GetMaxDependencyLayer() + 1, 0.0f);

		if (ImGui::Begin("Render Graph Visualizer"))
		{
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ImDrawList* drawList = window->DrawList;

			for (const auto& node : dependecyGraph.GetNodes())
			{
				const char* name = node.PassNode->Specifications.GetDebugName().c_str();
				ImVec2 textSize = ImGui::CalcTextSize(name);

				const ImVec2 textPosition(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
				ImVec2 cursorPosition = ImVec2(offsets[node.DependencyLayer], node.DependencyLayer * ImGui::GetFontSize() * 2.0f);

				FLARE_CORE_ASSERT(node.DependencyLayer < (uint32_t)offsets.size());

				offsets[node.DependencyLayer] += textSize.x + 100.0f;

				drawList->AddText(textPosition + cursorPosition, UINT32_MAX, name);
			}

			ImGui::End();
		}
	}
}
