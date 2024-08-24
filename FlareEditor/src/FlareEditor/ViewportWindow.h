#pragma once

#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Viewport.h"

#include <glm/glm.hpp>

#include <string>
#include <string_view>

namespace Flare
{
	class Event;
	class Scene;
	class SceneRenderer;
	class ViewportWindow : public RefCounted<ViewportWindow>
	{
	public:
		ViewportWindow(const Scope<SceneRenderer>& sceneRenderer, std::string_view name);
		virtual ~ViewportWindow() = default;
	public:
		virtual void OnAttach();

		virtual void OnRenderImGui();
		virtual void OnRenderViewport();
		virtual void OnEvent(Event& event) {}

		virtual void OnAddRenderPasses();

		Viewport& GetViewport() { return m_Viewport; }
		const Viewport& GetViewport() const { return m_Viewport; }

		const std::string& GetName() const { return m_Name; }

		inline const bool HasFocusChanged() const { return m_PreviousFocusState != m_IsFocused; }
		inline const bool IsFocused() const { return m_IsFocused; }

		inline void RequestFocus() { m_WindowFocusRequested = true; }

		void SetScene(const Ref<Scene>& scene) { m_Scene = scene; }
		void SetMaximized(bool maximized);

		void PrepareViewport();
	protected:
		Ref<Scene> GetScene() const;

		void BeginImGui();
		void RenderViewportBuffer(const Ref<Texture>& texture);
		void EndImGui();

		virtual void OnViewportChanged();
	private:
		void BuildRenderGraph();
	public:
		bool ShowWindow;
	protected:
		std::string m_Name;
		Ref<Scene> m_Scene;

		const Scope<SceneRenderer>& m_SceneRenderer;

		Viewport m_Viewport;

		bool m_Maximized = false;
		bool m_PreviousFocusState;
		bool m_IsFocused;
		bool m_IsHovered;
		bool m_WindowFocusRequested;
		bool m_IsVisible;

		glm::ivec2 m_RelativeMousePosition;
		glm::ivec2 m_ViewportOffset;
	};
}