#include "ViewportWindow.h"

#include "Flare/Renderer/RenderCommand.h"
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
		m_RenderData.IsEditorCamera = useEditorCamera;
	}

	void ViewportWindow::OnRenderViewport()
	{
		if (Scene::GetActive() == nullptr)
			return;

		PrepareViewport();

		if (m_RenderData.Viewport.Size != glm::ivec2(0))
		{
			m_FrameBuffer->Bind();

			OnClear();

			Scene::GetActive()->OnBeforeRender(m_RenderData);
			Scene::GetActive()->OnRender(m_RenderData);

			m_FrameBuffer->Unbind();
		}
	}

	const RenderData& ViewportWindow::GetRenderData() const
	{
		return m_RenderData;
	}

	void ViewportWindow::PrepareViewport()
	{
		if (m_RenderData.Viewport.Size != glm::ivec2(0))
		{
			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();

			if (frameBufferSpecs.Width != m_RenderData.Viewport.Size.x || frameBufferSpecs.Height != m_RenderData.Viewport.Size.y)
			{
				m_FrameBuffer->Resize(m_RenderData.Viewport.Size.x, m_RenderData.Viewport.Size.y);
				OnViewportChanged();
			}

			RenderCommand::SetViewport(0, 0, m_RenderData.Viewport.Size.x, m_RenderData.Viewport.Size.y);
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

		if (m_FrameBuffer != nullptr)
		{
			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();
			ImVec2 imageSize = ImVec2((float) frameBufferSpecs.Width, (float) frameBufferSpecs.Height);
			ImGui::Image((ImTextureID)m_FrameBuffer->GetColorAttachmentRendererId(0), windowSize, ImVec2(0, 1), ImVec2(1, 0));
		}

		if (m_RenderData.Viewport.Size == glm::ivec2(0))
		{
			m_RenderData.Viewport.Size = newViewportSize;
			CreateFrameBuffer();
			OnViewportChanged();
		}
		else if (newViewportSize != m_RenderData.Viewport.Size)
		{
			m_RenderData.Viewport.Size = newViewportSize;
		}

		m_RenderData.Viewport.Position = glm::ivec2(windowPosition.x, windowPosition.y) + m_ViewportOffset - (glm::ivec2)window->GetProperties().Position;
	}

	void ViewportWindow::EndImGui()
	{
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void ViewportWindow::CreateFrameBuffer()
	{
		FrameBufferSpecifications specifications(m_RenderData.Viewport.Size.x, m_RenderData.Viewport.Size.y, {
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
