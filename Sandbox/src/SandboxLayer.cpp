#include "SandboxLayer.h"

#include "Flare.h"
#include "Flare/Core/Application.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include <imgui.h>

namespace Flare
{
	SandboxLayer::SandboxLayer()
		: Layer("Sandbox"), m_ProjectionMatrix(glm::mat4(0.0f)), m_InverseProjection(glm::mat4(0.0f))
	{
	}

	void SandboxLayer::OnAttach()
	{
		m_QuadShader = Shader::Create("QuadShader.glsl");

		RenderCommand::SetClearColor(0.2f, 0.3f, 0.6f, 1.0f);

		CalculateProjection(m_CameraSize);
	}

	void SandboxLayer::OnUpdate(float deltaTime)
	{
		Renderer2D::Begin(m_QuadShader, m_ProjectionMatrix);

		int width = 20;
		int height = 20;

		glm::vec4 cornerColors[4] =
		{
			glm::vec4(0.94f, 0.15f, 0.09f, 1.0f),
			glm::vec4(0.13f, 0.98f, 0.14f, 1.0f),
		};

		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				glm::vec4 color0 = glm::lerp(cornerColors[0], cornerColors[1], (float)x / (float)width);
				glm::vec4 color1 = glm::lerp(cornerColors[0], cornerColors[1], (float)y / (float)height);

				Renderer2D::DrawQuad(glm::vec3(x - width / 2, y - height / 2, 0), glm::vec2(0.8f), (color0 + color1) / 2.0f);
			}
		}

		Renderer2D::End();
	}

	void SandboxLayer::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& resizeEvent) -> bool
		{
			CalculateProjection(m_CameraSize);
			return false;
		});
	}

	void SandboxLayer::OnImGUIRender()
	{
		ImGui::Begin("Sandbox Layer Window");

		ImGui::Button("Button");

		ImGui::End();
	}

	void SandboxLayer::CalculateProjection(float size)
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