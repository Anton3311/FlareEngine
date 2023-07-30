#include "ViewportWindow.h"

#include "Flare/Renderer/RenderCommand.h"

#include "FlareEditor/EditorContext.h"

#include <imgui.h>

namespace Flare
{
	ViewportWindow::ViewportWindow(std::string_view name, bool useEditorCamera)
		: m_Name(name), m_IsFocused(false), m_IsHovered(false), m_RelativeMousePosition(glm::ivec2(0))
	{
		m_RenderData.IsEditorCamera = useEditorCamera;
	}

	void ViewportWindow::OnRenderViewport()
	{
		if (m_RenderData.ViewportSize != glm::u32vec2(0))
		{
			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();

			if (frameBufferSpecs.Width != m_RenderData.ViewportSize.x || frameBufferSpecs.Height != m_RenderData.ViewportSize.y)
			{
				m_FrameBuffer->Resize(m_RenderData.ViewportSize.x, m_RenderData.ViewportSize.y);
				OnViewportResize();
			}

			RenderCommand::SetViewport(0, 0, m_RenderData.ViewportSize.x, m_RenderData.ViewportSize.y);

			m_FrameBuffer->Bind();

			OnClear();

			EditorContext::GetActiveScene()->OnBeforeRender(m_RenderData);
			EditorContext::GetActiveScene()->OnRender(m_RenderData);

			m_FrameBuffer->Unbind();
		}
	}

	void ViewportWindow::BeginImGui()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(m_Name.c_str());

		m_IsHovered = ImGui::IsWindowHovered();
		m_IsFocused = ImGui::IsWindowFocused();

		ImGuiIO& io = ImGui::GetIO();

		ImVec2 cursorPosition = ImGui::GetCursorPos();
		ImVec2 windowPosition = ImGui::GetWindowPos();
		ImVec2 windowSize = ImGui::GetContentRegionAvail();
		glm::u32vec2 newViewportSize = glm::u32vec2((int32_t)windowSize.x, (int32_t)windowSize.y);
		m_ViewportOffset = glm::uvec2(cursorPosition.x, cursorPosition.y);

		m_RelativeMousePosition = glm::uvec2(
			(uint32_t)(io.MousePos.x - windowPosition.x),
			(uint32_t)(io.MousePos.y - windowPosition.y)) - m_ViewportOffset;

		m_RelativeMousePosition.y = m_RenderData.ViewportSize.y - m_RelativeMousePosition.y;

		if (m_FrameBuffer != nullptr)
		{
			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();
			ImVec2 imageSize = ImVec2(frameBufferSpecs.Width, frameBufferSpecs.Height);
			ImGui::Image((ImTextureID)m_FrameBuffer->GetColorAttachmentRendererId(0), windowSize, ImVec2(0, 1), ImVec2(1, 0));
		}

		if (m_RenderData.ViewportSize == glm::u32vec2(0))
		{
			m_RenderData.ViewportSize = newViewportSize;
			CreateFrameBuffer();
			OnViewportResize();
		}
		else if (newViewportSize != m_RenderData.ViewportSize)
		{
			m_RenderData.ViewportSize = newViewportSize;
		}
	}

	void ViewportWindow::EndImGui()
	{
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void ViewportWindow::CreateFrameBuffer()
	{
		FrameBufferSpecifications specifications(m_RenderData.ViewportSize.x, m_RenderData.ViewportSize.y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::NoFiltering },
		});

		m_FrameBuffer = FrameBuffer::Create(specifications);
	}

	void ViewportWindow::OnClear()
	{
		RenderCommand::Clear();
	}

	void ViewportWindow::OnRenderImGui()
	{
		BeginImGui();
		EndImGui();
	}
}
