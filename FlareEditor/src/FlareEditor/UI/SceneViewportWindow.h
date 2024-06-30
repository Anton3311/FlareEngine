#pragma once

#include "Flare/Scene/Scene.h"
#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/Material.h"

#include "FlareECS/Entity/Entity.h"

#include "FlareEditor/Guizmo.h"
#include "FlareEditor/ViewportWindow.h"
#include "FlareEditor/EditorCamera.h"
#include "FlareEditor/EditorCameraController.h"

namespace Flare
{
	class SceneViewportWindow : public ViewportWindow
	{
	public:
		enum class ViewportOverlay
		{
			Default,
			Normal,
			Depth,
		};

		SceneViewportWindow(EditorCamera& camera, std::string_view name = "Scene Viewport");

		virtual void OnAttach() override;

		virtual void OnRenderViewport() override;
		virtual void OnViewportChanged() override;
		virtual void OnRenderImGui() override;
		virtual void OnEvent(Event& event) override;
		virtual void OnAddRenderPasses() override;
	protected:
		virtual void CreateFrameBuffer() override;
		virtual void OnClear() override;
	private:
		void RenderWindowContents();
		void RenderToolBar();
		void RenderGrid();

		void HandleAssetDragAndDrop(AssetHandle handle);
		std::optional<Entity> GetEntityUnderCursor() const;
	private:
		GuizmoMode m_Guizmo;
		EditorCamera& m_Camera;
		EditorCameraController m_CameraController;
		bool m_IsToolbarHovered;

		ViewportOverlay m_Overlay;
		Ref<Material> m_SelectionOutlineMaterial;
		Ref<Material> m_GridMaterial;
	};
}