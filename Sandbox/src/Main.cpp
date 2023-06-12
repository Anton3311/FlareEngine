#include <Flare/Core/Application.h>
#include <Flare/Renderer/RenderCommand.h>
#include <Flare/Core/EntryPoint.h>

#include <Flare/Renderer/Shader.h>
#include <Flare/Renderer2D/Renderer2D.h>
#include <Flare/Renderer/Texture.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace Flare;

class SandboxApplication : public Application
{
public:
	SandboxApplication()
	{
		Renderer2D::Initialize();
		m_Texture = Texture::Create("Texture.png");
		m_QuadShader = Shader::Create("QuadShader.glsl");

		CalculateProjection(2.0f);

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
		Renderer2D::End();
	}
private:
	Ref<Shader> m_QuadShader;
	Ref<Texture> m_Texture;
	glm::mat4 m_ProjectionMatrix;
};

Scope<Application> Flare::CreateFlareApplication(Flare::CommandLineArguments arguments)
{
	return CreateScope<SandboxApplication>();
}