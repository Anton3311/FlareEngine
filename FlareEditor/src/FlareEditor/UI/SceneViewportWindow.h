#pragma once

#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/VertexArray.h"

#include "FlareECS/Entity/Entity.h"

#include "FlareEditor/ViewportWindow.h"
#include "FlareEditor/EditorCamera.h"

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

		SceneViewportWindow(EditorCamera& camera);

		virtual void OnAttach() override;

		virtual void OnRenderViewport() override;
		virtual void OnViewportChanged() override;
		virtual void OnRenderImGui() override;
		virtual void OnEvent(Event& event) override;
	protected:
		virtual void CreateFrameBuffer() override;
		virtual void OnClear() override;
	private:
		void RenderWindowContents();
		void RenderToolBar();
		void RenderGrid();

		Entity GetEntityUnderCursor() const;
	private:
		EditorCamera& m_Camera;
		bool m_IsToolbarHovered;

		ViewportOverlay m_Overlay;
		Ref<Material> m_SelectionOutlineMaterial;
		Ref<Material> m_GridMaterial;
		Ref<FrameBuffer> m_ScreenBuffer;
	};
}