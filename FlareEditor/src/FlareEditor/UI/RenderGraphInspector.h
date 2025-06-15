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
		void RenderRenderGraph();
		void HandleDragging();
		void OnClose();
	private:
		Entity m_CurrentViewport;
		bool m_IsVisible = false;

		std::vector<glm::vec2> m_NodePositions;
		std::vector<float> m_LayerOffsets;

		ImVec2 m_MoveStartPosition = ImVec2(0.0f, 0.0f);
		ImVec2 m_Offset = ImVec2(0.0f, 0.0f);
		bool m_IsDragging = false;
	};
}