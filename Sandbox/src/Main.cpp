#include "Flare.h"

#include "Flare/Core/EntryPoint.h"
#include "Flare/Renderer2D/Renderer2D.h"

using namespace Flare;

class SandboxApplication : public Application
{
public:
	SandboxApplication()
	{
		Renderer2D::Initialize();
		m_Texture = Texture::Create("Texture.png");
		m_QuadShader = Shader::Create("QuadShader.glsl");

		CalculateProjection(m_CameraSize);

		RenderCommand::SetClearColor(0.1f, 0.2f, 0.3f, 1);
	}

	void CalculateProjection(float size)
	{
		const auto& windowProperties = m_Window->GetProperties();

		float width = windowProperties.Width;
		float height = windowProperties.Height;

		float halfSize = size / 2;
		float aspectRation = width / height;

		m_ProjectionMatrix = glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, -0.1f, 10.0f);
		m_InverseProjection = glm::inverse(m_ProjectionMatrix);
	}

	glm::vec3 ScreenPositionToWorldPosition(glm::vec2 screenPosition)
	{
		glm::vec2 windowSize = glm::vec2(m_Window->GetProperties().Width, m_Window->GetProperties().Height);
		glm::vec2 position = screenPosition / windowSize - glm::vec2(0.5f);
		return m_CameraSize * (m_InverseProjection * glm::vec4(position.x, -position.y, 0.0, 0.0));
	}

	virtual void OnEvent(Event& event) override
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) -> bool
		{
			CalculateProjection(m_CameraSize);
			return true;
		});

		dispatcher.Dispatch<MouseMoveEvent>([this](MouseMoveEvent& event) -> bool
		{
			m_QuadPosition = ScreenPositionToWorldPosition(event.GetPosition());
			return true;
		});

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event) -> bool
		{
			if (event.GetMouseCode() == MouseCode::ButtonLeft)
				m_ColorIndex = (m_ColorIndex + 1) % 4;
			else if (event.GetMouseCode() == MouseCode::ButtonRight)
				m_ColorIndex = (4 + m_ColorIndex - 1) % 4;

			return true;
		});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& event) -> bool
		{
			FLARE_INFO("Released {0}", (uint32_t)event.GetMouseCode());
			return true;
		});

		dispatcher.Dispatch<MouseScrollEvent>([this](MouseScrollEvent& event) -> bool
		{
			FLARE_INFO("Scrolled {0} {1}", event.GetOffset().x, event.GetOffset().y);
			return true;
		});
	}

	~SandboxApplication()
	{
		Renderer2D::Shutdown();
	}
public:
	virtual void OnUpdate() override
	{
		RenderCommand::Clear();

		Renderer2D::Begin(m_QuadShader, m_ProjectionMatrix);
		Renderer2D::DrawQuad(glm::vec3(0.5f, 0.5f, 0.0f), glm::vec2(0.4f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
		Renderer2D::DrawQuad(glm::vec3(-0.8f, -0.8f, 0.0f), glm::vec2(0.1f), glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));
		Renderer2D::DrawQuad(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.2f), glm::vec4(0.1f, 0.8f, 0.2f, 1.0f));
		Renderer2D::DrawQuad(glm::vec3(-0.5f, 0.2f, 0.0f), glm::vec2(0.08f), glm::vec4(0.1f, 0.8f, 0.2f, 1.0f));
		Renderer2D::DrawQuad(glm::vec3(-0.5f, -0.3f, 0.0f), glm::vec2(0.12f), glm::vec4(0.1f, 0.8f, 0.2f, 1.0f));
		Renderer2D::DrawQuad(glm::vec3(-0.2f, 0.7f, 0.0f), glm::vec2(0.4f, 0.2f), glm::vec4(0.8f, 0.2f, 0.1f, 1.0f));

		Renderer2D::DrawQuad(glm::vec3(0.0f), glm::vec2(0.2f), m_Texture, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
		Renderer2D::DrawQuad(glm::vec3(-0.1f), glm::vec2(1), m_Texture, glm::vec4(1.0f), glm::vec2(4));

		Renderer2D::DrawQuad(m_QuadPosition, glm::vec3(0.4f), m_Texture, m_Colors[m_ColorIndex]);

		Renderer2D::End();
	}
private:
	Ref<Shader> m_QuadShader;
	Ref<Texture> m_Texture;
	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_InverseProjection;

	glm::vec3 m_QuadPosition;

	glm::vec4 m_Colors[4] =
	{
		glm::vec4(1.0f),
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4(0.0f, 0.0f, 01.0f, 1.0f),
	};

	int32_t m_ColorIndex = 0;
	float m_CameraSize = 2.0f;
};

Scope<Application> Flare::CreateFlareApplication(Flare::CommandLineArguments arguments)
{
	return CreateScope<SandboxApplication>();
}
