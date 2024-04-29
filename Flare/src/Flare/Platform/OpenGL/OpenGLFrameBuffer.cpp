#include "OpenGLFrameBuffer.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Flare
{
    static GLenum FrameBufferTextureForamtToOpenGLInternalFormat(FrameBufferTextureFormat format)
    {
        switch (format)
        {
        case FrameBufferTextureFormat::RGB8:
            return GL_RGB8;
        case FrameBufferTextureFormat::R11G11B10:
            return GL_R11F_G11F_B10F;
        case FrameBufferTextureFormat::RGBA8:
            return GL_RGBA8;
        case FrameBufferTextureFormat::RedInteger:
            return GL_R32I;
        case FrameBufferTextureFormat::RF32:
            return GL_R32F;
        }

        FLARE_CORE_ASSERT(false, "Unhanded frame buffer texture format");
        return 0;
    }

    static GLenum FrameBufferTextureFormatToOpenGLFormat(FrameBufferTextureFormat format)
    {
        switch (format)
        {
        case FrameBufferTextureFormat::RGB8:
        case FrameBufferTextureFormat::R11G11B10:
            return GL_RGB;
        case FrameBufferTextureFormat::RGBA8:
            return GL_RGBA;
        case FrameBufferTextureFormat::RedInteger:
            return GL_RED_INTEGER;
        case FrameBufferTextureFormat::RF32:
            return GL_RED;
        }

        FLARE_CORE_ASSERT(false, "Unhanded frame buffer texture format");
        return 0;
    }

    OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferSpecifications& specifications)
        : m_Specifications(specifications)
    {
        m_ColorAttachments.resize(m_Specifications.Attachments.size());
        Create();
    }
    
    OpenGLFrameBuffer::~OpenGLFrameBuffer()
    {
        glDeleteFramebuffers(1, &m_Id);
        glDeleteTextures((int32_t)m_ColorAttachments.size(), m_ColorAttachments.data());
    }

    void OpenGLFrameBuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
    }

    void OpenGLFrameBuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFrameBuffer::Resize(uint32_t width, uint32_t height)
    {
        FLARE_PROFILE_FUNCTION();
        glDeleteFramebuffers(1, &m_Id);
        glDeleteTextures((int32_t)m_ColorAttachments.size(), m_ColorAttachments.data());

        m_Specifications.Width = width;
        m_Specifications.Height = height;

        Create();
    }

    uint32_t OpenGLFrameBuffer::GetAttachmentsCount() const
    {
        return (uint32_t)m_ColorAttachments.size();
    }

    uint32_t OpenGLFrameBuffer::GetColorAttachmentsCount() const
    {
        if (m_DepthAttachmentIndex)
            return (uint32_t)m_Specifications.Attachments.size() - 1;
        
        return (uint32_t)m_Specifications.Attachments.size();
    }

    std::optional<uint32_t> OpenGLFrameBuffer::GetDepthAttachmentIndex() const
    {
        return m_DepthAttachmentIndex;
    }

    void OpenGLFrameBuffer::ClearAttachment(uint32_t index, const void* value)
    {
        FLARE_PROFILE_FUNCTION();
        FLARE_CORE_ASSERT(index < m_ColorAttachments.size());

        GLenum type = 0;
        switch (m_Specifications.Attachments[index].Format)
        {
        case FrameBufferTextureFormat::RGB8:
        case FrameBufferTextureFormat::RGBA8:
            type = GL_UNSIGNED_BYTE;
            break;
        case FrameBufferTextureFormat::RF32:
            type = GL_FLOAT;
            break;
        case FrameBufferTextureFormat::RedInteger:
            type = GL_INT;
            break;
        default:
            FLARE_CORE_ASSERT(false);
        }

        glClearTexImage(m_ColorAttachments[index], 0, FrameBufferTextureFormatToOpenGLFormat(m_Specifications.Attachments[index].Format), type, &value);
    }

    void OpenGLFrameBuffer::ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, void* pixelOutput)
    {
        FLARE_PROFILE_FUNCTION();

        FLARE_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size());
        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

        glReadPixels(x, y, 1, 1, FrameBufferTextureFormatToOpenGLFormat(m_Specifications.Attachments[attachmentIndex].Format), GL_INT, pixelOutput);
    }

    void OpenGLFrameBuffer::BindAttachmentTexture(uint32_t attachment, uint32_t slot)
    {
        FLARE_CORE_ASSERT(attachment < (uint32_t)m_ColorAttachments.size());
        glBindTextureUnit(slot, m_ColorAttachments[attachment]);
    }

    void OpenGLFrameBuffer::Create()
    {
        FLARE_CORE_ASSERT(m_Specifications.Width != 0 && m_Specifications.Height != 0);

        glCreateFramebuffers(1, &m_Id);
        glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
        glCreateTextures(GL_TEXTURE_2D, (int32_t)m_ColorAttachments.size(), m_ColorAttachments.data());

        for (size_t i = 0; i < m_ColorAttachments.size(); i++)
        {
            if (IsDepthFormat(m_Specifications.Attachments[i].Format))
                AttachDepthTexture((uint32_t)i);
            else
                AttachColorTexture((uint32_t)i);
        }

        FLARE_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Frame buffer is incomplete");

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }

    void OpenGLFrameBuffer::AttachColorTexture(uint32_t index)
    {
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[index]);
        glTexImage2D(GL_TEXTURE_2D, 0,
            FrameBufferTextureForamtToOpenGLInternalFormat(m_Specifications.Attachments[index].Format),
            m_Specifications.Width, m_Specifications.Height, 0,
            FrameBufferTextureFormatToOpenGLFormat(m_Specifications.Attachments[index].Format),
            GL_UNSIGNED_BYTE, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0 + index), GL_TEXTURE_2D, m_ColorAttachments[index], 0);
    }

    void OpenGLFrameBuffer::AttachDepthTexture(uint32_t index)
    {
        m_DepthAttachmentIndex = index;

        glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[index]);

        uint32_t format = 0;
        switch (m_Specifications.Attachments[index].Format)
        {
        case FrameBufferTextureFormat::Depth24Stencil8:
            format = GL_DEPTH24_STENCIL8;
            break;
        default:
            FLARE_CORE_ASSERT(false, "Attachment format is not depth format");
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexStorage2D(GL_TEXTURE_2D, 1, format, m_Specifications.Width, m_Specifications.Height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_ColorAttachments[index], 0);
    }
}
