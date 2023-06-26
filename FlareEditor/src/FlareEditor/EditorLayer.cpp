#include "EditorLayer.h"

#include "Flare.h"
#include "Flare/Core/Application.h"

#include "Flare/Renderer2D/Renderer2D.h"

#include <imgui.h>

namespace Flare
{
	EditorLayer::EditorLayer()
		: Layer("EditorLayer"), m_ProjectionMatrix(glm::mat4(0.0f)), m_InverseProjection(glm::mat4(0.0f))
	{
	}

	void EditorLayer::OnAttach()
	{
		m_QuadShader = Shader::Create("QuadShader.glsl");

		Ref<Window> window = Application::GetInstance().GetWindow();
		uint32_t width = window->GetProperties().Width;
		uint32_t height = window->GetProperties().Height;

		FrameBufferSpecifications specifications(width, height, {
			{FrameBufferTextureFormat::RGB8, TextureWrap::Clamp, TextureFiltering::NoFiltering }
			});

		m_FrameBuffer = FrameBuffer::Create(specifications);

		RenderCommand::SetClearColor(0.04f, 0.07f, 0.1f, 1.0f);

		CalculateProjection(m_CameraSize);
	}

	void EditorLayer::OnUpdate(float deltaTime)
	{
		m_FrameBuffer->Bind();
		RenderCommand::Clear();

		m_PreviousFrameTime = deltaTime;

		Renderer2D::ResetStats();
		Renderer2D::Begin(m_QuadShader, m_ProjectionMatrix);

		glm::vec4 cornerColors[4] =
		{
			glm::vec4(0.94f, 0.15f, 0.09f, 1.0f),
			glm::vec4(0.13f, 0.12f, 0.98f, 1.0f),
		};

		for (int32_t y = 0; y < m_Height; y++)
		{
			for (int32_t x = 0; x < m_Width; x++)
			{
				glm::vec4 color0 = glm::lerp(cornerColors[0], cornerColors[1], (float)x / (float)m_Width);
				glm::vec4 color1 = glm::lerp(cornerColors[0], cornerColors[1], (float)y / (float)m_Height);

				Renderer2D::DrawQuad(glm::vec3(x - m_Width / 2, y - m_Height / 2, 0), glm::vec2(0.8f), (color0 + color1) / 2.0f);
			}
		}

		Renderer2D::End();
		m_FrameBuffer->Unbind();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& resizeEvent) -> bool
		{
			CalculateProjection(m_CameraSize);
			return false;
		});
	}

	void EditorLayer::OnImGUIRender()
	{
		static bool fullscreen = true;
		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		if (fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspaceFlags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
			windowFlags |= ImGuiWindowFlags_NoBackground;

		static bool open = true;
		ImGui::Begin("DockSpaceTest", &open, windowFlags);

		ImGuiID dockspaceId = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

		{
			ImGui::Begin("Renderer 2D");

			const auto& stats = Renderer2D::GetStats();
			ImGui::Text("Quads %d", stats.QuadsCount);
			ImGui::Text("Draw Calls %d", stats.DrawCalls);
			ImGui::Text("Vertices %d", stats.GetTotalVertexCount());

			ImGui::Text("Frame time %f", m_PreviousFrameTime);
			ImGui::Text("FPS %f", 1.0f / m_PreviousFrameTime);

			ImGui::End();
		}

		{
			ImGui::Begin("Settings");

			Ref<Window> window = Application::GetInstance().GetWindow();

			bool vsync = window->GetProperties().VSyncEnabled;
			if (ImGui::Checkbox("VSync", &vsync))
			{
				window->SetVSync(vsync);
			}

			ImGui::DragInt("Width", &m_Width, 1, 100);
			ImGui::DragInt("Height", &m_Height);

			if (ImGui::SliderFloat("Camera Size", &m_CameraSize, 1.0f, 100.0f))
				CalculateProjection(m_CameraSize);

			const FrameBufferSpecifications frameBufferSpecs = m_FrameBuffer->GetSpecifications();
			ImVec2 imageSize = ImVec2(frameBufferSpecs.Width / 4, frameBufferSpecs.Height / 4);
			ImGui::Image((ImTextureID)m_FrameBuffer->GetColorAttachmentRendererId(0), imageSize);

			ImGui::End();
		}

		ImGui::End();
	}

	void EditorLayer::CalculateProjection(float size)
	{
		const auto& windowProperties = Application::GetInstance().GetWindow()->GetProperties();

		float width = windowProperties.Width;
		float height = windowProperties.Height;

		float halfSize = size / 2;
		float aspectRation = width / height;

		m_ProjectionMatrix = glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, -0.1f, 10.0f);
		m_InverseProjection = glm::inverse(m_ProjectionMatrix);
	}
}