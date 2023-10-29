#include "Renderer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer2D/Renderer2D.h"

namespace Flare
{
	struct InstanceData
	{
		glm::mat4 Transform;
	};

	struct RenderableObject
	{
		Ref<Mesh> Mesh;
		Ref<Material> Material;
		glm::mat4 Transform;
	};

	struct RendererData
	{
		Viewport* MainViewport;
		Viewport* CurrentViewport;

		Ref<UniformBuffer> CameraBuffer;
		Ref<UniformBuffer> LightBuffer;
		Ref<VertexArray> FullscreenQuad;

		Ref<Texture> WhiteTexture;
		
		std::vector<Ref<RenderPass>> RenderPasses;
		RendererStatistics Statistics;

		std::vector<RenderableObject> Queue;

		Ref<VertexBuffer> InstanceBuffer;
		std::vector<InstanceData> InstanceDataBuffer;
		uint32_t MaxInstances = 1024;

		Ref<Mesh> CurrentInstencingMesh = nullptr;
	};
	
	RendererData s_RendererData;

	void Renderer::Initialize()
	{
		s_RendererData.MainViewport = nullptr;
		s_RendererData.CurrentViewport = nullptr;
		s_RendererData.InstanceBuffer = nullptr;

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
			0, 2, 3,
		};

		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(sizeof(vertices), (const void*)vertices);
		vertexBuffer->SetLayout({
			BufferLayoutElement("i_Position", ShaderDataType::Float2),
		});

		s_RendererData.FullscreenQuad = VertexArray::Create();
		s_RendererData.FullscreenQuad->SetIndexBuffer(IndexBuffer::Create(6, (const void*)indices));
		s_RendererData.FullscreenQuad->AddVertexBuffer(vertexBuffer);
		s_RendererData.FullscreenQuad->Unbind();

		{
			uint32_t whiteTextureData = 0xffffffff;
			s_RendererData.WhiteTexture = Texture::Create(1, 1, &whiteTextureData, TextureFormat::RGBA8);
		}

		s_RendererData.InstanceBuffer = VertexBuffer::Create(s_RendererData.MaxInstances * sizeof(InstanceData));
		s_RendererData.InstanceBuffer->SetLayout({
			{ "i_Transform", ShaderDataType::Matrix4x4 }
		});
	}

	void Renderer::Shutdown()
	{
		s_RendererData.FullscreenQuad = nullptr;
	}

	const RendererStatistics& Renderer::GetStatistics()
	{
		return s_RendererData.Statistics;
	}

	void Renderer::ClearStatistics()
	{
		s_RendererData.Statistics.DrawCallsCount = 0;
		s_RendererData.Statistics.DrawCallsSavedByInstances = 0;
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

	static void ApplyMaterialFeatures(MaterialFeatures features)
	{
		RenderCommand::SetDepthTestEnabled(features.DepthTesting);
		RenderCommand::SetCullingMode(features.Culling);
	}

	static bool CompareRenderableObjects(const RenderableObject& a, const RenderableObject& b)
	{
		if ((uint64_t)a.Material->Handle < (uint64_t)b.Material->Handle)
			return true;

		if (a.Material->Handle == b.Material->Handle)
		{
			if ((uint64_t)a.Mesh->Handle < (uint64_t)b.Mesh->Handle)
				return true;
		}

		return false;
	}

	void Renderer::Flush()
	{
		std::sort(s_RendererData.Queue.begin(), s_RendererData.Queue.end(), CompareRenderableObjects);

		Ref<Material> currentMaterial = nullptr;
		FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();

		for (const RenderableObject& object : s_RendererData.Queue)
		{
			if (object.Mesh->GetSubMesh().InstanceBuffer == nullptr)
				object.Mesh->SetInstanceBuffer(s_RendererData.InstanceBuffer);

			if (s_RendererData.CurrentInstencingMesh.get() != object.Mesh.get())
			{
				FlushInstances();
				s_RendererData.CurrentInstencingMesh = object.Mesh;
			}

			if (object.Material.get() != currentMaterial.get())
			{
				FlushInstances();

				currentMaterial = object.Material;
				ApplyMaterialFeatures(object.Material->Features);
				currentMaterial->SetShaderProperties();

				Ref<Shader> shader = object.Material->GetShader();
				if (shader == nullptr)
					continue;

				FrameBufferAttachmentsMask shaderOutputsMask = 0;
				for (uint32_t output : shader->GetOutputs())
					shaderOutputsMask |= (1 << output);

				s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(shaderOutputsMask);
			}

			auto& instanceData = s_RendererData.InstanceDataBuffer.emplace_back();
			instanceData.Transform = object.Transform;

			if (s_RendererData.InstanceDataBuffer.size() == (size_t)s_RendererData.MaxInstances)
				FlushInstances();
		}

		FlushInstances();
		s_RendererData.CurrentInstencingMesh = nullptr;

		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);

		s_RendererData.InstanceDataBuffer.clear();
		s_RendererData.Queue.clear();
	}

	void Renderer::FlushInstances()
	{
		size_t instancesCount = s_RendererData.InstanceDataBuffer.size();
		if (instancesCount == 0 || s_RendererData.CurrentInstencingMesh == nullptr)
			return;

		s_RendererData.InstanceBuffer->SetData(s_RendererData.InstanceDataBuffer.data(), sizeof(InstanceData) * instancesCount);

		RenderCommand::DrawInstanced(s_RendererData.CurrentInstencingMesh->GetSubMesh().MeshVertexArray, instancesCount);
		s_RendererData.Statistics.DrawCallsCount++;
		s_RendererData.Statistics.DrawCallsSavedByInstances += instancesCount - 1;

		s_RendererData.InstanceDataBuffer.clear();
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::DrawFullscreenQuad(const Ref<Material>& material)
	{
		material->SetShaderProperties();
		ApplyMaterialFeatures(material->Features);

		const ShaderOutputs& shaderOutputs = material->GetShader()->GetOutputs();

		FrameBufferAttachmentsMask shaderOutputsMask = 0;
		for (uint32_t output : shaderOutputs)
			shaderOutputsMask |= (1 << output);

		FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();
		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(shaderOutputsMask);

		RenderCommand::DrawIndexed(s_RendererData.FullscreenQuad);
		s_RendererData.Statistics.DrawCallsCount++;

		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);
	}

	void Renderer::DrawMesh(const Ref<VertexArray>& mesh, const Ref<Material>& material, size_t indicesCount)
	{
		material->SetShaderProperties();
		ApplyMaterialFeatures(material->Features);

		const ShaderOutputs& shaderOutputs = material->GetShader()->GetOutputs();

		FrameBufferAttachmentsMask shaderOutputsMask = 0;
		for (uint32_t output : shaderOutputs)
			shaderOutputsMask |= (1 << output);

		FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();
		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(shaderOutputsMask);

		RenderCommand::DrawIndexed(mesh, indicesCount == SIZE_MAX ? mesh->GetIndexBuffer()->GetCount() : indicesCount);
		s_RendererData.Statistics.DrawCallsCount++;

		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);
	}

	void Renderer::DrawMesh(const Ref<Mesh>& mesh, const Ref<Material>& material, const glm::mat4& transform)
	{
		RenderableObject& object = s_RendererData.Queue.emplace_back();
		object.Material = material;
		object.Mesh = mesh;
		object.Transform = transform;
	}

	void Renderer::AddRenderPass(Ref<RenderPass> pass)
	{
		s_RendererData.RenderPasses.push_back(pass);
	}

	void Renderer::ExecuteRenderPasses()
	{
		FLARE_CORE_ASSERT(s_RendererData.CurrentViewport);
		RenderingContext context(s_RendererData.CurrentViewport->RenderTarget, s_RendererData.CurrentViewport->RTPool);

		for (Ref<RenderPass>& pass : s_RendererData.RenderPasses)
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

	Ref<Texture> Renderer::GetWhiteTexture()
	{
		return s_RendererData.WhiteTexture;
	}

	Ref<const VertexArray> Renderer::GetFullscreenQuad()
	{
		return s_RendererData.FullscreenQuad;
	}
}
