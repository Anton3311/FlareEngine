#include "ViewportWindow.h"

#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Core/Application.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Flare
{
	ViewportWindow::ViewportWindow(std::string_view name, bool useEditorCamera)
		: m_Name(name),
		m_IsFocused(false),
		m_IsHovered(false),
		IsWindowVisible(true),
		m_RelativeMousePosition(glm::ivec2(0)),
		m_ViewportOffset(glm::uvec2(0))
	{
		m_Viewport.FrameData.IsEditorCamera = useEditorCamera;
	}

	void ViewportWindow::OnRenderViewport()
	{
		if (!IsWindowVisible)
			return;

		Ref<Scene> scene = Scene::GetActive();
		if (scene == nullptr)
			return;

		PrepareViewport();

		if (m_Viewport.GetSize() != glm::ivec2(0))
		{
			scene->OnBeforeRender(m_Viewport);

			m_Viewport.RenderTarget->Bind();

			OnClear();

			Renderer::BeginScene(m_Viewport);
			scene->OnRender(m_Viewport);
			Renderer::EndScene();

			m_Viewport.RenderTarget->Unbind();
		}
	}

	void ViewportWindow::PrepareViewport()
	{
		if (m_Viewport.GetSize() != glm::ivec2(0))
		{
			const FrameBufferSpecifications frameBufferSpecs = m_Viewport.RenderTarget->GetSpecifications();

			if (frameBufferSpecs.Width != m_Viewport.GetSize().x || frameBufferSpecs.Height != m_Viewport.GetSize().y)
			{
				m_Viewport.RenderTarget->Resize(m_Viewport.GetSize().x, m_Viewport.GetSize().y);
				OnViewportChanged();
			}

			m_Viewport.RTPool.SetRenderTargetsSize(m_Viewport.GetSize());
			RenderCommand::SetViewport(0, 0, m_Viewport.GetSize().x, m_Viewport.GetSize().y);
		}
	}

	void ViewportWindow::BeginImGui()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(m_Name.c_str(), &IsWindowVisible);

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

		if (m_Viewport.RenderTarget != nullptr)
		{
			const FrameBufferSpecifications frameBufferSpecs = m_Viewport.RenderTarget->GetSpecifications();
			ImVec2 imageSize = ImVec2((float) frameBufferSpecs.Width, (float) frameBufferSpecs.Height);
			ImGui::Image((ImTextureID)m_Viewport.RenderTarget->GetColorAttachmentRendererId(0), windowSize, ImVec2(0, 1), ImVec2(1, 0));
		}

		bool changed = false;
		glm::ivec2 viewportSize = m_Viewport.GetSize();
		glm::ivec2 viewportPosition = m_Viewport.GetPosition();

		if (m_Viewport.GetSize() == glm::ivec2(0))
		{
			viewportSize = newViewportSize;
			changed = true;
		}
		else if (newViewportSize != m_Viewport.GetSize())
		{
			viewportSize = newViewportSize;
			changed = true;
		}

		glm::ivec2 position = glm::ivec2(windowPosition.x, windowPosition.y) + m_ViewportOffset - (glm::ivec2)window->GetProperties().Position;
		if (position != viewportPosition)
		{
			viewportPosition = position;
			changed = true;
		}

		if (changed)
		{
			bool shouldCreateFrameBuffers = m_Viewport.GetSize() == glm::ivec2(0);
			m_Viewport.Resize(viewportPosition, viewportSize);

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

		m_Viewport.RenderTarget = FrameBuffer::Create(specifications);
	}

	void ViewportWindow::OnClear()
	{
		RenderCommand::Clear();
	}

	void ViewportWindow::OnRenderImGui()
	{
		if (!IsWindowVisible)
			return;

		BeginImGui();
		EndImGui();
	}
}
