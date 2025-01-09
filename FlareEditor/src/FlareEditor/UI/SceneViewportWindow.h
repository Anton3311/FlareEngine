#pragma once

#include "Flare/AssetManager/Asset.h"

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

	struct SceneViewportFeatures
	{
		bool GizmosEnabled = true;
	};

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
			EditorSelection& editorSelection,
			std::string_view name = "Scene Viewport",
			ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None);

		virtual void OnAttach() override;

		virtual void OnRenderViewport(const World& renderWorld) override;
		virtual void OnViewportChanged() override;
		virtual void OnRenderImGui() override;
		virtual void OnEvent(Event& event) override;
		virtual void OnAddRenderPasses(RenderGraph& renderGraph) override;

		inline EditorCamera& GetEditorCamera() { return m_EditorCamera; }
		inline const EditorCamera& GetEditorCamera() const { return m_EditorCamera; }

		inline void SetFeatures(const SceneViewportFeatures& features) { m_Features = features; }
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
		EditorSelection& m_EditorSelection;

		SceneViewportFeatures m_Features;

		GuizmoMode m_Guizmo = GuizmoMode::None;
		TransformationSpace m_TransformationSpace = TransformationSpace::World;

		EditorCamera m_EditorCamera;
		EditorCameraController m_CameraController;
		bool m_IsToolbarHovered = false;

		Scope<RenderGraphInspector> m_RenderGraphInspector = nullptr;

		ViewportOverlay m_Overlay = ViewportOverlay::Default;
	};
}