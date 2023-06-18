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

		RenderCommand::SetClearColor(0.17f, 0.69f, 0.80f, 1);
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

		float ration = windowSize.x / windowSize.y;
		return 2.0f * (m_InverseProjection * glm::vec4(position.x, -position.y, 0.0, 1.0));
	}

	virtual void OnEvent(Event& event) override
	{
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) -> bool
		{
			CalculateProjection(m_CameraSize);
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
	virtual void OnUpdate(float deltaTime) override
	{
		RenderCommand::Clear();
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
private:
	Ref<Shader> m_QuadShader;
	Ref<Texture> m_Texture;

	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_InverseProjection;

	glm::vec4 m_Colors[4] =
	{
		glm::vec4(1.0f),
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
		glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
		glm::vec4(0.0f, 0.0f, 01.0f, 1.0f),
	};

	int32_t m_ColorIndex = 0;
	float m_CameraSize = 24.0f;
};

Scope<Application> Flare::CreateFlareApplication(Flare::CommandLineArguments arguments)
{
	return CreateScope<SandboxApplication>();
}
