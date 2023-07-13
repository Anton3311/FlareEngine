#include "SceneViewportWindow.h"

namespace Flare
{
	void SceneViewportWindow::OnRenderViewport()
	{
		if (m_RenderData.IsEditorCamera)
		{
			m_RenderData.Camera.ViewMatrix = m_Camera.GetViewMatrix();
			m_RenderData.Camera.ProjectionMatrix = m_Camera.GetProjectionMatrix();
			m_RenderData.Camera.CalculateViewProjection();
		}

		ViewportWindow::OnRenderViewport();
	}

	void SceneViewportWindow::OnViewportResize()
	{
		m_Camera.RecalculateProjection(m_RenderData.ViewportSize);
	}
}
