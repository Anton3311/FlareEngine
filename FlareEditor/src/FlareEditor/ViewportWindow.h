#pragma once

#include "Flare/Renderer/RenderData.h"

#include "FlareECS/Entity/Entity.h"

#include <glm/glm.hpp>

#include <string>
#include <string_view>

namespace Flare
{
	class Event;
	class RenderGraph;
	class Scene;
	class SceneRenderer;
	class Texture;
	class World;

	class ViewportWindow : public RefCounted<ViewportWindow>
	{
	public:
		ViewportWindow(const Scope<SceneRenderer>& sceneRenderer, std::string_view name);
		virtual ~ViewportWindow();

		virtual void OnAttach();

		virtual void OnRenderImGui();
		virtual void OnRenderViewport(const World& renderWorld);
		virtual void OnEvent(Event& event) {}

		virtual void OnAddRenderPasses(RenderGraph& renderGraph);

		void SetScene(const Ref<Scene>& scene) { m_Scene = scene; }
		void SetMaximized(bool maximized);

		inline const std::string& GetName() const { return m_Name; }

		inline const bool HasFocusChanged() const { return m_PreviousFocusState != m_IsFocused; }
		inline const bool IsFocused() const { return m_IsFocused; }

		inline void RequestFocus() { m_WindowFocusRequested = true; }
		inline Entity GetViewportEntity() const { return m_ViewportEntity; }
	protected:
		Ref<Scene> GetScene() const;

		void BeginImGui();
		void RenderViewportBuffer(const Ref<Texture>& texture);
		void EndImGui();

		virtual void OnViewportChanged();

		void BuildRenderGraph(RenderGraph& renderGraph);
	public:
		bool ShowWindow;
	protected:
		std::string m_Name;
		Ref<Scene> m_Scene;

		const Scope<SceneRenderer>& m_SceneRenderer;

		Entity m_ViewportEntity;

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