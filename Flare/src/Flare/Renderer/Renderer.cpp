#include "Renderer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer2D/Renderer2D.h"

namespace Flare
{
	struct RendererData
	{
		Viewport* MainViewport;
		Viewport* CurrentViewport;

		Ref<UniformBuffer> CameraBuffer;
		Ref<UniformBuffer> LightBuffer;
		Ref<VertexArray> FullscreenQuad;

		std::vector<Scope<RenderPass>> RenderPasses;
	};

	RendererData s_RendererData;

	void Renderer::Initialize()
	{
		s_RendererData.MainViewport = nullptr;
		s_RendererData.CurrentViewport = nullptr;

		s_RendererData.CameraBuffer = UniformBuffer::Create(sizeof(CameraData), 0);
		s_RendererData.LightBuffer = UniformBuffer::Create(sizeof(LightData), 1);

		float vertices[] = {
			-1, -1,
			-1,  1,
			 1,  1,
			 1, -1,
		};

		uint32_t indices[] = {
			0, 1, 2,
			2, 0, 3,
		};

		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(sizeof(vertices), (const void*)vertices);
		vertexBuffer->SetLayout({
			BufferLayoutElement("i_Position", ShaderDataType::Float2),
		});

		s_RendererData.FullscreenQuad = VertexArray::Create();
		s_RendererData.FullscreenQuad->SetIndexBuffer(IndexBuffer::Create(6, (const void*)indices));
		s_RendererData.FullscreenQuad->AddVertexBuffer(vertexBuffer);
		s_RendererData.FullscreenQuad->Unbind();
	}

	void Renderer::Shutdown()
	{
		s_RendererData.FullscreenQuad = nullptr;
	}

	void Renderer::SetMainViewport(Viewport& viewport)
	{
		s_RendererData.MainViewport = &viewport;
	}

	void Renderer::BeginScene(Viewport& viewport)
	{
		s_RendererData.CurrentViewport = &viewport;
		s_RendererData.CameraBuffer->SetData(&viewport.FrameData.Camera, sizeof(CameraData), 0);
		s_RendererData.LightBuffer->SetData(&viewport.FrameData.Light, sizeof(LightData), 0);
	}

	void Renderer::EndScene()
	{
	}

	Ref<const VertexArray> Renderer::GetFullscreenQuad()
	{
		return s_RendererData.FullscreenQuad;
	}

	void Renderer::DrawMesh(const Ref<VertexArray>& mesh, const Ref<Material>& material, size_t indicesCount)
	{
		material->SetShaderParameters();
		RenderCommand::DrawIndexed(mesh, indicesCount == SIZE_MAX ? mesh->GetIndexBuffer()->GetCount() : indicesCount);
	}

	void Renderer::DrawMesh(const Ref<Mesh>& mesh, const Ref<Material>& material, const glm::mat4& transform)
	{
		DrawMesh(mesh->GetSubMesh().MeshVertexArray, material);
	}

	void Renderer::AddRenderPass(Scope<RenderPass> pass)
	{
		s_RendererData.RenderPasses.push_back(std::move(pass));
	}

	void Renderer::ExecuteRenderPasses()
	{
		FLARE_CORE_ASSERT(s_RendererData.CurrentViewport);
		RenderingContext context(s_RendererData.CurrentViewport->RenderTarget, s_RendererData.CurrentViewport->RTPool);

		for (Scope<RenderPass>& pass : s_RendererData.RenderPasses)
			pass->OnRender(context);
	}

	Viewport& Renderer::GetMainViewport()
	{
		FLARE_CORE_ASSERT(s_RendererData.MainViewport);
		return *s_RendererData.MainViewport;
	}

	Viewport& Renderer::GetCurrentViewport()
	{
		FLARE_CORE_ASSERT(s_RendererData.CurrentViewport);
		return *s_RendererData.CurrentViewport;
	}
}
