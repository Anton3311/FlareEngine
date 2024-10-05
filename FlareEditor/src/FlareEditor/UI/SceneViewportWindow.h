#pragma once

#include "FlareEditor/SceneViewSettings.h"
#include "FlareEditor/Guizmo.h"
#include "FlareEditor/ViewportWindow.h"
#include "FlareEditor/EditorCamera.h"
#include "FlareEditor/EditorCameraController.h"

#include "FlareEditor/UI/RenderGraphInspector.h"

namespace Flare
{
	namespace Math
	{
		struct AffineTransform;
	}

	class Scene;
	class SceneViewportWindow : public ViewportWindow
	{
	public:
		enum class ViewportOverlay
		{
			Default,
			Depth,
		};

		SceneViewportWindow(const Scope<SceneRenderer>& sceneRenderer,
			SceneViewSettings& sceneViewSettings,
			std::string_view name = "Scene Viewport");

		virtual void OnAttach() override;

		virtual void OnRenderViewport() override;
		virtual void OnViewportChanged() override;
		virtual void OnRenderImGui() override;
		virtual void OnEvent(Event& event) override;
		virtual void OnAddRenderPasses() override;

		inline EditorCamera& GetEditorCamera() { return m_EditorCamera; }
		inline const EditorCamera& GetEditorCamera() const { return m_EditorCamera; }
	private:
		void RenderWindowContents();
		void RenderToolBar();

		void HandleAssetDragAndDrop(AssetHandle handle);
		void HandleGuizmo();

		bool HandleTransformation(Math::AffineTransform& localTransform,
			const Math::AffineTransform* globalTransform,
			const glm::mat4* parentTransform) const;
	private:
		SceneViewSettings& m_SceneViewSettings;

		GuizmoMode m_Guizmo = GuizmoMode::None;
		TransformationSpace m_TransformationSpace = TransformationSpace::World;

		EditorCamera m_EditorCamera;
		EditorCameraController m_CameraController;
		bool m_IsToolbarHovered = false;

		Scope<RenderGraphInspector> m_RenderGraphInspector = nullptr;

		ViewportOverlay m_Overlay = ViewportOverlay::Default;
	};
}