#include "PCH.h"

#include "RenderGraphInspector.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/Renderer.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
	static RenderGraphInspector s_Instance;

	void RenderGraphInspector::OnRenderImGui()
	{
		FLARE_PROFILE_FUNCTION();

		if (!m_IsVisible)
			return;

		if (ImGui::Begin("Render Graph Inspector", &m_IsVisible))
		{
			World& renderWorld = Renderer::GetRenderWorld();
			const Viewport* currentViewport = renderWorld.TryGetEntityComponent<const Viewport>(m_CurrentViewport);
			
			const char* previewText = "None";

			if (currentViewport)
			{
				previewText = currentViewport->Name.c_str();
			}

			if (ImGui::BeginCombo("Viewport", previewText))
			{
				Renderer::GetViewportsQuery().ForEachChunk([&](QueryChunk chunk, ComponentView<const Viewport> viewports)
				{
					for (size_t i = 0; i < chunk.GetEntityCount(); i++)
					{
						const Viewport& viewport = viewports[i];
						Entity viewportId = chunk.GetEntityId(i);
						if (ImGui::MenuItem(viewport.Name.c_str(), nullptr))
						{
							m_CurrentViewport = viewportId;
							m_Offset = ImVec2(0.0f, 0.0f);
						}
					}
				});

				ImGui::EndCombo();
			}

			ImVec2 viewportSize = ImGui::GetContentRegionAvail();
			if (ImGui::BeginChild("RenderGraphViewport", viewportSize, false, ImGuiWindowFlags_NoMove))
			{
				RenderRenderGraph();
				ImGui::EndChild();
			}

			ImGui::End();
		}

		if (!m_IsVisible)
		{
			OnClose();
		}
	}

	void RenderGraphInspector::Show()
	{
		m_IsVisible = true;
	}

	RenderGraphInspector& RenderGraphInspector::GetInstance()
	{
		return s_Instance;
	}

	void RenderGraphInspector::RenderRenderGraph()
	{
		FLARE_PROFILE_FUNCTION();

		constexpr float SPACING_BETWEEN_LAYERS = 60.0f;
		constexpr float SPACING_BETWEEN_NODES = 30.0f;

		World& renderWorld = Renderer::GetRenderWorld();

		bool isViewportValid = renderWorld.IsEntityAlive(m_CurrentViewport);
		if (!isViewportValid)
			return;

		const ViewportRenderGraph* viewportRenderGraph = renderWorld.TryGetEntityComponent<const ViewportRenderGraph>(m_CurrentViewport);
		if (!viewportRenderGraph)
			return;

		const DependencyGraph& dependencyGraph = viewportRenderGraph->Graph->GetDependencyGraph();
		if (dependencyGraph.GetMaxDependencyLayer() == 0)
			return;

		HandleDragging();

		const ImGuiStyle& style = ImGui::GetStyle();
		const ImU32 textColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
		const ImU32 lineColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);

		m_LayerOffsets.resize(dependencyGraph.GetMaxDependencyLayer() + 1);
		m_NodePositions.resize(dependencyGraph.GetGraphNodes().size());

		m_NodePositions.assign(m_NodePositions.size(), glm::vec2(0.0f, 0.0f));
		m_LayerOffsets.assign(m_LayerOffsets.size(), 0.0f);

		const auto& nodes = dependencyGraph.GetGraphNodes();
		float textHeight = ImGui::GetFontSize();

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* drawList = window->DrawList;

		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			if (!nodes[nodeIndex].PassNode->Enabled)
				continue;

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

			for (size_t dependencyIndex : node.Dependencies)
			{
				glm::vec2 dependencyPosition = m_NodePositions[dependencyIndex];

				ImVec2 start = ImVec2(dependencyPosition.x, dependencyPosition.y + textHeight / 2.0f);
				ImVec2 end = ImVec2(nodeNameTextCenter.x, nodeNameTextCenter.y - textHeight / 2.0f) + textSize / 2.0f;

				start += window->DC.CursorPos;
				end += window->DC.CursorPos;

				drawList->AddLine(start, end, lineColor);
			}
		}
	}

	void RenderGraphInspector::HandleDragging()
	{
		bool isHoveringViewport = ImGui::IsWindowHovered(ImGuiWindowFlags_ChildWindow);
		if (!m_IsDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			if (isHoveringViewport)
			{
				m_MoveStartPosition = ImGui::GetMousePos() - m_Offset;
				m_IsDragging = true;
			}
		}

		if (m_IsDragging && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			m_Offset = ImGui::GetMousePos() - m_MoveStartPosition;
		}

		if (m_IsDragging && !ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDragging(ImGuiMouseButton_Left))
		{
			m_IsDragging = false;
		}
	}

	void RenderGraphInspector::OnClose()
	{
		FLARE_PROFILE_FUNCTION();
		m_NodePositions.clear();
		m_LayerOffsets.clear();
	}
}
