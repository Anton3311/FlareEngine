#include "PCH.h"

#include "RenderGraphInspector.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/Renderer.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/UI/EditorGUI.h"

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

			ImGui::SetNextItemWidth(300.0f);
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

			ImGui::SameLine();

			if (ImGui::Button("Settings"))
			{
				ImGui::OpenPopup("RenderGraphInspectorSettings");
			}

			ImRect settingsButtonRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

			constexpr float POPUP_CONTENT_WIDTH = 300.0f;

			ImGui::SetNextWindowPos(ImVec2(settingsButtonRect.Min.x, settingsButtonRect.Max.y));
			if (ImGui::BeginPopup("RenderGraphInspectorSettings"))
			{
				if (EditorGUI::BeginPropertyGrid(POPUP_CONTENT_WIDTH))
				{
					EditorGUI::PropertyName("Node Spacing");
					ImGui::PushID("RenderGraphInspectorNodeSpacing");
					ImGui::DragFloat("", &m_Settings.SpacingBetweenNodes, 1.0f, 0.0f, 300.0f);
					ImGui::PopID();

					EditorGUI::PropertyName("Layer Spacing");
					ImGui::PushID("RenderGraphInspectorLayerSpacing");
					ImGui::DragFloat("", &m_Settings.SpacingBetweenLayers, 1.0f, 0.0f, 300.0f);
					ImGui::PopID();

					EditorGUI::EndPropertyGrid();
				}

				if (ImGui::Button("Reset View"))
				{
					m_Offset = ImVec2(0.0f, 0.0f);
					ImGui::CloseCurrentPopup();
				}

				ImGui::End();
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

		m_Layers.resize(dependencyGraph.GetMaxDependencyLayer() + 1);
		m_NodeState.resize(dependencyGraph.GetGraphNodes().size());

		m_NodeState.assign(m_NodeState.size(), NodeState());
		m_Layers.assign(m_Layers.size(), Layer{});

		const auto& nodes = dependencyGraph.GetGraphNodes();
		float textHeight = ImGui::GetFontSize();

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* drawList = window->DrawList;

		// Compute width of all layers
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			if (!nodes[nodeIndex].PassNode->Enabled)
				continue;

			const auto& node = nodes[nodeIndex];
			const char* name = node.PassNode->Specifications.GetDebugName().c_str();
			ImVec2 textSize = ImGui::CalcTextSize(name);

			m_Layers[node.DependencyLayer].TotalTextWidth += textSize.x;
			m_Layers[node.DependencyLayer].NodeCount++;
		}

		float maxWidth = 0.0f;
		for (const Layer& layer : m_Layers)
		{
			maxWidth = glm::max(maxWidth, layer.TotalTextWidth + (glm::max(layer.NodeCount, 1u) - 1) * m_Settings.SpacingBetweenNodes);
		}

		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			if (!nodes[nodeIndex].PassNode->Enabled)
				continue;

			const auto& node = nodes[nodeIndex];
			const char* name = node.PassNode->Specifications.GetDebugName().c_str();
			ImVec2 textSize = ImGui::CalcTextSize(name);

			ImVec2 textPosition(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
			ImVec2 nodeNameTextCenter = ImVec2(m_Layers[node.DependencyLayer].Offset, node.DependencyLayer * m_Settings.SpacingBetweenLayers);
			nodeNameTextCenter += m_Offset;

			Layer& layer = m_Layers[node.DependencyLayer];
			float layerWidth = layer.TotalTextWidth + (glm::max(layer.NodeCount, 1u) - 1) * m_Settings.SpacingBetweenNodes;
			float centerAlignmentOffset = (maxWidth - layerWidth) * 0.5f;

			layer.Offset += textSize.x + m_Settings.SpacingBetweenNodes;

			nodeNameTextCenter.x += centerAlignmentOffset;

			m_NodeState[nodeIndex].Position.x = nodeNameTextCenter.x + textSize.x / 2.0f;
			m_NodeState[nodeIndex].Position.y = nodeNameTextCenter.y + textSize.y / 2.0f;
			m_NodeState[nodeIndex].TextRect = {
				textPosition + nodeNameTextCenter,
				textPosition + nodeNameTextCenter + textSize,
			};

			for (size_t dependencyIndex : node.Dependencies)
			{
				NodeState nodeState = m_NodeState[dependencyIndex];

				ImVec2 start = ImVec2(nodeState.Position.x, nodeState.Position.y + textHeight / 2.0f);
				ImVec2 end = ImVec2(nodeNameTextCenter.x, nodeNameTextCenter.y - textHeight / 2.0f) + textSize / 2.0f;

				start += window->DC.CursorPos;
				end += window->DC.CursorPos;

				drawList->AddLine(start, end, lineColor);
			}
		}

		// Now draw the node names

		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			if (!nodes[nodeIndex].PassNode->Enabled)
				continue;

			ImRect textRect = m_NodeState[nodeIndex].TextRect;

			const char* name = nodes[nodeIndex].PassNode->Specifications.GetDebugName().c_str();

			ImU32 nameTextColor = 0;
			ImU32 textBackgroundColor = 0;

			switch (nodes[nodeIndex].PassNode->Specifications.GetType())
			{
			case RenderGraphPassType::Graphics:
			case RenderGraphPassType::Other:
				nameTextColor = textColor;
				textBackgroundColor = ImGui::GetColorU32(ImGuiCol_FrameBg);
				break;
			case RenderGraphPassType::Compute:
				nameTextColor = 0xffffffff;
				textBackgroundColor = ImGui::GetColorU32(ImGuiTheme::PrimaryVariant);
				break;
			default:
				FLARE_VERIFY_UNREACHABLE();
			}

			drawList->AddRectFilled(textRect.Min - style.FramePadding,
				textRect.Max + style.FramePadding,
				textBackgroundColor,
				style.FrameRounding);

			drawList->AddText(textRect.Min, nameTextColor, name);
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
		m_NodeState.clear();
		m_Layers.clear();
	}
}
