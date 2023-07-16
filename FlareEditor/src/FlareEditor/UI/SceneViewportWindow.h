#pragma once

#include "FlareEditor/ViewportWindow.h"
#include "FlareEditor/EditorCamera.h"

namespace Flare
{
	class SceneViewportWindow : public ViewportWindow
	{
	public:
		SceneViewportWindow(EditorCamera& camera)
			: ViewportWindow("Scene Viewport", true), m_Camera(camera) {}

		virtual void OnRenderViewport() override;
		virtual void OnViewportResize() override;
		virtual void OnRenderImGui() override;
	protected:
		virtual void CreateFrameBuffer() override;
		virtual void OnClear() override;
	private:
		EditorCamera& m_Camera;
	};
}