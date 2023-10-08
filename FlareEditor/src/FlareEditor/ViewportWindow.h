#pragma once

#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Viewport.h"

#include <glm/glm.hpp>

#include <string>
#include <string_view>

namespace Flare
{
	class ViewportWindow
	{
	public:
		ViewportWindow(std::string_view name, bool useEditorCamera = false);
		virtual ~ViewportWindow() = default;
	public:
		virtual void OnRenderImGui();
		virtual void OnRenderViewport();

		const RenderData& GetRenderData() const { return m_Viewport.FrameData; }

		Viewport& GetViewport() { return m_Viewport; }
		const Viewport& GetViewport() const { return m_Viewport; }

		void PrepareViewport();

		void SetViewProjection(const glm::mat4& projection) { m_Viewport.FrameData.Camera.Projection = projection; }
	protected:
		void BeginImGui();
		void EndImGui();

		virtual void CreateFrameBuffer();
		virtual void OnClear();
		virtual void OnViewportChanged() {}
	protected:
		std::string m_Name;
		Ref<FrameBuffer> m_FrameBuffer;
		Viewport m_Viewport;
		bool m_IsFocused;
		bool m_IsHovered;
		glm::ivec2 m_RelativeMousePosition;
		glm::ivec2 m_ViewportOffset;
	};
}