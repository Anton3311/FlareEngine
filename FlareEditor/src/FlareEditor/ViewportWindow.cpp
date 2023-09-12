#include "ViewportWindow.h"

#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Core/Application.h"

#include <imgui.h>

namespace Flare
{
	ViewportWindow::ViewportWindow(std::string_view name, bool useEditorCamera)
		: m_Name(name), m_IsFocused(false),
		m_IsHovered(false),
		m_RelativeMousePosition(glm::ivec2(0)),
		m_ViewportOffset(glm::uvec2(0))
	{
		m_Viewport.FrameData.IsEditorCamera = useEditorCamera;
	}

	void ViewportWindow::OnRenderViewport()
	{
		if (Scene::GetActive() == nullptr)
			return;

		PrepareViewport();

		if (m_Viewport.GetSize() != glm::ivec2(0))
		{
			m_FrameBuffer->Bind();

			OnClear();

			Scene::GetActive()->OnBeforeRender(m_Viewport.FrameData);
			Scene::GetActive()->OnRender(m_Viewport.FrameData);

			m_FrameBuffer->Unbind();
		}
	}

	void ViewportWindow::PrepareViewport()
	{
		Renderer::SetCurrentViewport(m_Viewport);
		if (m_Viewport.GetSize() != glm::ivec2(0))
		{
			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();

			if (frameBufferSpecs.Width != m_Viewport.GetSize().x || frameBufferSpecs.Height != m_Viewport.GetSize().y)
			{
				m_FrameBuffer->Resize(m_Viewport.GetSize().x, m_Viewport.GetSize().y);
				OnViewportChanged();
			}

			RenderCommand::SetViewport(0, 0, m_Viewport.GetSize().x, m_Viewport.GetSize().y);
		}
	}

	void ViewportWindow::BeginImGui()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(m_Name.c_str());

		m_IsHovered = ImGui::IsWindowHovered();
		m_IsFocused = ImGui::IsWindowFocused();

		ImGuiIO& io = ImGui::GetIO();

		Ref<Window> window = Application::GetInstance().GetWindow();

		ImVec2 cursorPosition = ImGui::GetCursorPos();
		ImVec2 windowPosition = ImGui::GetWindowPos();
		ImVec2 windowSize = ImGui::GetContentRegionAvail();
		glm::ivec2 newViewportSize = glm::u32vec2((int32_t)windowSize.x, (int32_t)windowSize.y);
		m_ViewportOffset = glm::ivec2(cursorPosition.x, cursorPosition.y);

		m_RelativeMousePosition = glm::ivec2(
			(int32_t)(io.MousePos.x - windowPosition.x),
			(int32_t)(io.MousePos.y - windowPosition.y)) - m_ViewportOffset;

		m_RelativeMousePosition.y = newViewportSize.y - m_RelativeMousePosition.y;

		if (m_FrameBuffer != nullptr)
		{
			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();
			ImVec2 imageSize = ImVec2((float) frameBufferSpecs.Width, (float) frameBufferSpecs.Height);
			ImGui::Image((ImTextureID)m_FrameBuffer->GetColorAttachmentRendererId(0), windowSize, ImVec2(0, 1), ImVec2(1, 0));
		}

		bool changed = false;
		ViewportRect viewportRect = m_Viewport.GetRect();

		if (m_Viewport.GetSize() == glm::ivec2(0))
		{
			viewportRect.Size = newViewportSize;
			changed = true;
		}
		else if (newViewportSize != m_Viewport.GetSize())
		{
			viewportRect.Size = newViewportSize;
			changed = true;
		}

		glm::ivec2 position = glm::ivec2(windowPosition.x, windowPosition.y) + m_ViewportOffset - (glm::ivec2)window->GetProperties().Position;
		if (position != viewportRect.Position)
		{
			viewportRect.Position = position;
			changed = true;
		}

		if (changed)
		{
			bool shouldCreateFrameBuffers = m_Viewport.GetSize() == glm::ivec2(0);
			m_Viewport.Resize(viewportRect);

			if (shouldCreateFrameBuffers)
				CreateFrameBuffer();

			OnViewportChanged();
		}
	}

	void ViewportWindow::EndImGui()
	{
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void ViewportWindow::CreateFrameBuffer()
	{
		FrameBufferSpecifications specifications(m_Viewport.GetSize().x, m_Viewport.GetSize().y, {
			{ FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::Closest },
			{ FrameBufferTextureFormat::Depth, TextureWrap::Clamp, TextureFiltering::Closest },
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
