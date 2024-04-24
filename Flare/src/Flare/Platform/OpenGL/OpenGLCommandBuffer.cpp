#include "OpenGLCommandBuffer.h"

#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/RenderCommand.h"

#include "Flare/Platform/OpenGL/OpenGLGPUTimer.h"

#include <glad/glad.h>

namespace Flare
{
	OpenGLCommandBuffer::~OpenGLCommandBuffer()
	{
		FLARE_CORE_ASSERT(m_CurrentRenderTarget == nullptr);
	}

	void OpenGLCommandBuffer::BeginRenderTarget(const Ref<FrameBuffer> frameBuffer)
	{
		FLARE_CORE_ASSERT(m_CurrentRenderTarget == nullptr);
		m_CurrentRenderTarget = As<OpenGLFrameBuffer>(frameBuffer);
		m_CurrentRenderTarget->Bind();
	}

	void OpenGLCommandBuffer::EndRenderTarget()
	{
		FLARE_CORE_ASSERT(m_CurrentRenderTarget);

		m_CurrentRenderTarget->Unbind();
		m_CurrentRenderTarget = nullptr;
	}

	void OpenGLCommandBuffer::ClearColorAttachment(Ref<FrameBuffer> frameBuffer, uint32_t index, const glm::vec4& clearColor)
	{
		FLARE_CORE_ASSERT(index < frameBuffer->GetAttachmentsCount());
		frameBuffer->Bind();

		frameBuffer->SetWriteMask(1 << index);
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClear(GL_COLOR_BUFFER_BIT);

		frameBuffer->Unbind();

		if (m_CurrentRenderTarget)
		{
			m_CurrentRenderTarget->Bind();
		}
	}

	void OpenGLCommandBuffer::ClearDepthAttachment(Ref<FrameBuffer> frameBuffer, float depth)
	{
		frameBuffer->Bind();

		// Transform to [-1, 1]
		glClearDepth((GLdouble)(depth * 2.0f - 1.0f));
		glClear(GL_DEPTH_BUFFER_BIT);

		frameBuffer->Unbind();

		if (m_CurrentRenderTarget)
		{
			m_CurrentRenderTarget->Bind();
		}
	}

	void OpenGLCommandBuffer::ApplyMaterial(const Ref<const Material>& material)
	{
		FLARE_CORE_ASSERT(m_CurrentRenderTarget);

		Ref<Shader> shader = material->GetShader();
		FLARE_CORE_ASSERT(shader);

		RenderCommand::ApplyMaterial(material);

		FrameBufferAttachmentsMask shaderOutputsMask = 0;
		for (uint32_t output : shader->GetOutputs())
			shaderOutputsMask |= (1 << output);

		m_CurrentRenderTarget->SetWriteMask(shaderOutputsMask);
	}

	void OpenGLCommandBuffer::SetViewportAndScisors(Math::Rect viewportRect)
	{
		glViewport((int32_t)viewportRect.Min.x, (int32_t)viewportRect.Min.y, (int32_t)viewportRect.GetWidth(), (int32_t)viewportRect.GetHeight());
		glScissor((int32_t)viewportRect.Min.x, (int32_t)viewportRect.Min.y, (int32_t)viewportRect.GetWidth(), (int32_t)viewportRect.GetHeight());
	}

	void OpenGLCommandBuffer::DrawIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t baseInstance, uint32_t instanceCount)
	{
		RenderCommand::DrawInstancesIndexed(mesh, subMeshIndex, instanceCount, baseInstance);
	}

	void OpenGLCommandBuffer::Blit(Ref<FrameBuffer> source, uint32_t sourceAttachment, Ref<FrameBuffer> destination, uint32_t destinationAttachment, TextureFiltering filter)
	{
		FLARE_CORE_ASSERT(source && destination);
		FLARE_CORE_ASSERT(destinationAttachment < destination->GetAttachmentsCount());
		FLARE_CORE_ASSERT(sourceAttachment < source->GetAttachmentsCount());

		Ref<OpenGLFrameBuffer> sourceFrameBuffer = As<OpenGLFrameBuffer>(source);
		Ref<OpenGLFrameBuffer> destinationFrameBuffer = As<OpenGLFrameBuffer>(destination);

		const auto& sourceSpec = source->GetSpecifications();
		const auto& destinationSpec = destination->GetSpecifications();

		bool sourceAttachmentIsDepth = IsDepthFormat(sourceSpec.Attachments[sourceAttachment].Format);
		bool destinationAttachmentIsDepth = IsDepthFormat(destinationSpec.Attachments[destinationAttachment].Format);

		FLARE_CORE_ASSERT(sourceAttachmentIsDepth == destinationAttachmentIsDepth);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFrameBuffer->GetId());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destinationFrameBuffer->GetId());

		glReadBuffer(GL_COLOR_ATTACHMENT0 + sourceAttachment);
		glDrawBuffer(GL_COLOR_ATTACHMENT0 + destinationAttachment);

		GLenum blitFilter = GL_NEAREST;
		switch (filter)
		{
		case TextureFiltering::Closest:
			blitFilter = GL_NEAREST;
			break;
		case TextureFiltering::Linear:
			blitFilter = GL_LINEAR;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		glBlitFramebuffer(
			0, 0, 
			sourceSpec.Width, 
			sourceSpec.Height, 
			0, 0, 
			destinationSpec.Width,
			destinationSpec.Height,
			sourceAttachmentIsDepth
				? GL_DEPTH_BUFFER_BIT
				: GL_COLOR_BUFFER_BIT,
			blitFilter);

		destination->SetWriteMask(destination->GetWriteMask());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLCommandBuffer::StartTimer(Ref<GPUTimer> timer)
	{
		As<OpenGLGPUTImer>(timer)->Start();
	}

	void OpenGLCommandBuffer::StopTimer(Ref<GPUTimer> timer)
	{
		As<OpenGLGPUTImer>(timer)->Stop();
	}
}
