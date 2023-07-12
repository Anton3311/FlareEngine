#include "SceneViewportWindow.h"

namespace Flare
{
	void SceneViewportWindow::OnRenderViewport()
	{
		m_RenderData.Camera.Projection = m_Camera.GetViewProjection();
		ViewportWindow::OnRenderViewport();
	}

	void SceneViewportWindow::OnViewportResize()
	{
		m_Camera.RecalculateProjection(m_RenderData.ViewportSize);
	}
}
