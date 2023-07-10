#include "ViewportWindow.h"

#include "Flare/Renderer/RenderCommand.h"

#include "FlareEditor/EditorContext.h"

#include <imgui.h>

namespace Flare
{
	ViewportWindow::ViewportWindow(std::string_view name, bool useEditorCamera)
		: m_Name(name)
	{
		m_RenderData.IsEditorCamera = useEditorCamera;
	}

	void ViewportWindow::OnRenderViewport()
	{
		if (m_RenderData.ViewportSize != glm::u32vec2(0))
		{
			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();

			if (frameBufferSpecs.Width != m_RenderData.ViewportSize.x || frameBufferSpecs.Height != m_RenderData.ViewportSize.y)
				m_FrameBuffer->Resize(m_RenderData.ViewportSize.x, m_RenderData.ViewportSize.y);

			RenderCommand::SetViewport(0, 0, m_RenderData.ViewportSize.x, m_RenderData.ViewportSize.y);

			m_FrameBuffer->Bind();

			RenderCommand::Clear();

			EditorContext::GetActiveScene()->OnBeforeRender(m_RenderData);
			EditorContext::GetActiveScene()->OnRender(m_RenderData);
			m_FrameBuffer->Unbind();
		}
	}

	void ViewportWindow::OnRenderImGui()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(m_Name.c_str());

		ImVec2 windowSize = ImGui::GetContentRegionAvail();
		glm::u32vec2 newViewportSize = glm::u32vec2((uint32_t)windowSize.x, (uint32_t)windowSize.y);

		if (m_FrameBuffer != nullptr)
		{
			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();
			ImVec2 imageSize = ImVec2(frameBufferSpecs.Width, frameBufferSpecs.Height);
			ImGui::Image((ImTextureID)m_FrameBuffer->GetColorAttachmentRendererId(0), windowSize);
		}

		if (m_RenderData.ViewportSize == glm::u32vec2(0))
		{
			m_RenderData.ViewportSize = newViewportSize;
			FrameBufferSpecifications specifications(m_RenderData.ViewportSize.x, m_RenderData.ViewportSize.y, {
				{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::NoFiltering }
			});

			m_FrameBuffer = FrameBuffer::Create(specifications);
		}
		else if (newViewportSize != m_RenderData.ViewportSize)
		{
			m_RenderData.ViewportSize = newViewportSize;
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}
}
