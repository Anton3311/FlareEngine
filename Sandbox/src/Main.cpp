#include <Flare/Core/Application.h>
#include <Flare/Renderer/RenderCommand.h>
#include <Flare/Core/EntryPoint.h>

#include <glm/glm.hpp>

using namespace Flare;

class SandboxApplication : public Application
{
public:
	SandboxApplication()
	{
		glm::vec3 vertices[] =
		{
			{ -0.5f, -0.5f, 0.0f },
			{ -0.5f,  0.5f, 0.0f },
			{  0.5f,  0.5f, 0.0f },
			{  0.5f, -0.5f, 0.0f },
		};

		uint32_t indices[] =
		{
			0, 1, 2,
			0, 2, 3
		};

		m_Quad = VertexArray::Create();
		m_Vertices = VertexBuffer::Create();
		m_Indices = IndexBuffer::Create();
		
		BufferLayout layout = {
			BufferLayoutElement("Vertex", ShaderDataType::Float3),
		};

		m_Vertices->SetLayout(layout);
		m_Vertices->SetData(vertices, sizeof(vertices));

		m_Indices->SetData(indices, 6);

		m_Quad->SetIndexBuffer(m_Indices);
		m_Quad->AddVertexBuffer(m_Vertices);

		RenderCommand::SetClearColor(0.1f, 0.2f, 0.3f, 1);
	}

public:
	virtual void OnUpdate() override
	{
		RenderCommand::Clear();
		RenderCommand::DrawIndex(m_Quad);
	}
private:
	Ref<VertexArray> m_Quad;
	Ref<VertexBuffer> m_Vertices;
	Ref<IndexBuffer> m_Indices;
};

Scope<Application> Flare::CreateFlareApplication(Flare::CommandLineArguments arguments)
{
	return CreateScope<SandboxApplication>();
}