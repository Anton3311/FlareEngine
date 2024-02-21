#pragma once

#include "Flare/Scene/Scene.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Viewport.h"

#include "FlarePlatform/Event.h"

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
		virtual void OnAttach();

		virtual void OnRenderImGui();
		virtual void OnRenderViewport();
		virtual void OnEvent(Event& event) {}

		Viewport& GetViewport() { return m_Viewport; }
		const Viewport& GetViewport() const { return m_Viewport; }

		const std::string& GetName() const { return m_Name; }

		inline const bool HasFocusChanged() const { return m_PreviousFocusState != m_IsFocused; }
		inline const bool IsFocused() const { return m_IsFocused; }

		inline void RequestFocus() { m_WindowFocusRequested = true; }

		void SetScene(const Ref<Scene>& scene) { m_Scene = scene; }

		void PrepareViewport();
	protected:
		inline Ref<Scene> GetScene() const { return m_Scene == nullptr ? Scene::GetActive() : m_Scene; }

		void BeginImGui();
		void RenderViewportBuffer(const Ref<FrameBuffer>& buffer, uint32_t attachmentIndex);
		void EndImGui();

		virtual void CreateFrameBuffer();
		virtual void OnClear();
		virtual void OnViewportChanged() {}
	public:
		bool ShowWindow;
	protected:
		std::string m_Name;
		Ref<Scene> m_Scene;
		Viewport m_Viewport;
		bool m_PreviousFocusState;
		bool m_IsFocused;
		bool m_IsHovered;

		bool m_WindowFocusRequested;

		bool m_IsVisible;

		glm::ivec2 m_RelativeMousePosition;
		glm::ivec2 m_ViewportOffset;
	};
}