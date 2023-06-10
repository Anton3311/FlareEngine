#include <Flare/Core/Application.h>
#include <Flare/Renderer/RenderCommand.h>
#include <Flare/Core/EntryPoint.h>

#include <Flare/Renderer/Shader.h>
#include <Flare/Renderer2D/Renderer2D.h>

#include <glm/glm.hpp>

using namespace Flare;

class SandboxApplication : public Application
{
public:
	SandboxApplication()
	{
		Renderer2D::Initialize();
		m_QuadShader = Shader::Create("QuadShader.glsl");

		RenderCommand::SetClearColor(0.1f, 0.2f, 0.3f, 1);
	}

	~SandboxApplication()
	{
		Renderer2D::Shutdown();
	}
public:
	virtual void OnUpdate() override
	{
		RenderCommand::Clear();

		Renderer2D::Begin(m_QuadShader);
		Renderer2D::Submit(glm::vec2(0.5f), glm::vec2(0.4f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
		Renderer2D::Submit(glm::vec2(-0.8f), glm::vec2(0.1f), glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));
		Renderer2D::Submit(glm::vec2(-0.5f), glm::vec2(0.2f), glm::vec4(0.1f, 0.8f, 0.2f, 1.0f));
		Renderer2D::Submit(glm::vec2(-0.5f, 0.2f), glm::vec2(0.08f), glm::vec4(0.1f, 0.8f, 0.2f, 1.0f));
		Renderer2D::Submit(glm::vec2(-0.5f, -0.3f), glm::vec2(0.12f), glm::vec4(0.1f, 0.8f, 0.2f, 1.0f));
		Renderer2D::Submit(glm::vec2(-0.2f, 0.7f), glm::vec2(0.4f, 0.2f), glm::vec4(0.8f, 0.2f, 0.1f, 1.0f));
		Renderer2D::End();
	}
private:
	Ref<Shader> m_QuadShader;
};

Scope<Application> Flare::CreateFlareApplication(Flare::CommandLineArguments arguments)
{
	return CreateScope<SandboxApplication>();
}