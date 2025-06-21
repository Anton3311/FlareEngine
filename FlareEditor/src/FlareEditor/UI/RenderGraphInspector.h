#pragma once

#include "FlareECS/Entity/Entity.h"

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
		void OnRenderImGui();

		inline void SetVisible(bool visible) { m_IsVisible = visible; }

		void Show();
		static RenderGraphInspector& GetInstance();
	private:
		struct VisualizationSettings
		{
			float SpacingBetweenLayers = 60.0f;
			float SpacingBetweenNodes = 30.0f;
		};

		struct Layer
		{
			float Offset = 0.0f;
			float TotalTextWidth = 0.0f;
			uint32_t NodeCount = 0;
		};

		struct NodeState
		{
			ImVec2 Position;
			ImRect TextRect;
		};

		void RenderRenderGraph();
		void HandleDragging();
		void OnClose();
	private:
		Entity m_CurrentViewport;
		bool m_IsVisible = false;

		std::vector<NodeState> m_NodeState;
		std::vector<Layer> m_Layers;

		ImVec2 m_MoveStartPosition = ImVec2(0.0f, 0.0f);
		ImVec2 m_Offset = ImVec2(0.0f, 0.0f);
		bool m_IsDragging = false;

		VisualizationSettings m_Settings;
	};
}