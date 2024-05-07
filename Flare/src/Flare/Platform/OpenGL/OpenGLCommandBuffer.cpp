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

		SetAttachmentsWriteMask((1 << m_CurrentRenderTarget->GetAttachmentsCount()) - 1);
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
		Ref<OpenGLFrameBuffer> openGlFrameBuffer = As<OpenGLFrameBuffer>(frameBuffer);
		openGlFrameBuffer->Bind();

		SetAttachmentsWriteMask(1 << index);
		glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
		glClear(GL_COLOR_BUFFER_BIT);

		openGlFrameBuffer->Unbind();

		if (m_CurrentRenderTarget)
		{
			m_CurrentRenderTarget->Bind();
		}
	}

	void OpenGLCommandBuffer::ClearDepthAttachment(Ref<FrameBuffer> frameBuffer, float depth)
	{
		Ref<OpenGLFrameBuffer> openGlFrameBuffer = As<OpenGLFrameBuffer>(frameBuffer);
		openGlFrameBuffer->Bind();

		// Transform to [-1, 1]
		glClearDepth((GLdouble)(depth * 2.0f - 1.0f));
		glDepthMask(GL_TRUE);
		glClear(GL_DEPTH_BUFFER_BIT);

		openGlFrameBuffer->Unbind();

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

		RendererAPI::GetInstance()->ApplyMaterialProperties(material);

		auto shaderFeatures = material->GetShader()->GetFeatures();
		SetBlendMode(shaderFeatures.Blending);
		SetDepthComparisonFunction(shaderFeatures.DepthFunction);
		SetCullingMode(shaderFeatures.Culling);
		SetDepthTestEnabled(shaderFeatures.DepthTesting);
		SetBlendMode(shaderFeatures.Blending);
		SetDepthWriteEnabled(shaderFeatures.DepthWrite);

		uint32_t shaderOutputsMask = 0;
		for (uint32_t output : shader->GetOutputs())
			shaderOutputsMask |= (1 << output);

		SetAttachmentsWriteMask(shaderOutputsMask);
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

		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLCommandBuffer::DispatchCompute(Ref<ComputePipeline> pipeline, const glm::uvec3& groupCount)
	{
	}

	void OpenGLCommandBuffer::StartTimer(Ref<GPUTimer> timer)
	{
		As<OpenGLGPUTImer>(timer)->Start();
	}

	void OpenGLCommandBuffer::StopTimer(Ref<GPUTimer> timer)
	{
		As<OpenGLGPUTImer>(timer)->Stop();
	}

	void OpenGLCommandBuffer::SetAttachmentsWriteMask(uint32_t mask)
	{
        GLenum s_Attachments[32];

        uint32_t insertionIndex = 0;
        for (uint32_t i = 0; i < 32; i++)
        {
            if (HAS_BIT(mask, 1 << i))
            {
                s_Attachments[insertionIndex] = GL_COLOR_ATTACHMENT0 + i;
                insertionIndex++;
            }
        }

        glDrawBuffers((int32_t)insertionIndex, s_Attachments);
	}

	void OpenGLCommandBuffer::SetDepthTestEnabled(bool enabled)
	{
		if (enabled)
			glEnable(GL_DEPTH_TEST);
		else
			glDisable(GL_DEPTH_TEST);
	}

	void OpenGLCommandBuffer::SetCullingMode(CullingMode mode)
	{
		switch (mode)
		{
		case CullingMode::None:
			glDisable(GL_CULL_FACE);
			break;
		case CullingMode::Front:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case CullingMode::Back:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;
		}
	}

	void OpenGLCommandBuffer::SetDepthComparisonFunction(DepthComparisonFunction function)
	{
		switch (function)
		{
		case DepthComparisonFunction::Less:
			glDepthFunc(GL_LESS);
			break;
		case DepthComparisonFunction::Greater:
			glDepthFunc(GL_GREATER);
			break;
		case DepthComparisonFunction::LessOrEqual:
			glDepthFunc(GL_LEQUAL);
			break;
		case DepthComparisonFunction::GreaterOrEqual:
			glDepthFunc(GL_GEQUAL);
			break;
		case DepthComparisonFunction::Equal:
			glDepthFunc(GL_EQUAL);
			break;
		case DepthComparisonFunction::NotEqual:
			glDepthFunc(GL_NOTEQUAL);
			break;
		case DepthComparisonFunction::Always:
			glDepthFunc(GL_ALWAYS);
			break;
		case DepthComparisonFunction::Never:
			glDepthFunc(GL_LESS);
			break;
		}
	}

	void OpenGLCommandBuffer::SetDepthWriteEnabled(bool enabled)
	{
		if (enabled)
			glDepthMask(GL_TRUE);
		else
			glDepthMask(GL_FALSE);
	}

	void OpenGLCommandBuffer::SetBlendMode(BlendMode mode)
	{
		switch (mode)
		{
		case BlendMode::Opaque:
			glDisable(GL_BLEND);
			break;
		case BlendMode::Transparent:
			glEnable(GL_BLEND);
			break;
		}
	}
}
