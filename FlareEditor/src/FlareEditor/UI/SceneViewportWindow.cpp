#include "SceneViewportWindow.h"

#include "Flare/Renderer/RenderCommand.h"
#include "FlareEditor/EditorContext.h"

#include <imgui.h>

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

	void SceneViewportWindow::OnRenderImGui()
	{
		BeginImGui();

		ImGuiIO& io = ImGui::GetIO();
		if (io.MouseClicked[ImGuiMouseButton_Left] && m_FrameBuffer != nullptr && m_IsHovered && m_RelativeMousePosition.x >= 0 && m_RelativeMousePosition.y >= 0)
		{
			m_FrameBuffer->Bind();

			int32_t entityIndex;
			m_FrameBuffer->ReadPixel(1, m_RelativeMousePosition.x, m_RelativeMousePosition.y, &entityIndex);

			std::optional<Entity> entity = EditorContext::GetActiveScene()->GetECSWorld().GetRegistry().FindEntityByIndex(entityIndex);
			EditorContext::Instance.SelectedEntity = entity.value_or(Entity());

			m_FrameBuffer->Unbind();
		}

		EndImGui();
	}

	void SceneViewportWindow::CreateFrameBuffer()
	{
		FrameBufferSpecifications specifications(m_RenderData.ViewportSize.x, m_RenderData.ViewportSize.y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::NoFiltering },
			{ FrameBufferTextureFormat::RedInteger, TextureWrap::Clamp, TextureFiltering::NoFiltering },
		});

		m_FrameBuffer = FrameBuffer::Create(specifications);
	}

	void SceneViewportWindow::OnClear()
	{
		RenderCommand::Clear();
		m_FrameBuffer->ClearAttachment(1, INT32_MAX);
	}
}
