#include "OpenGLFrameBuffer.h"

#include "Flare/Core/Assert.h"

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
        case FrameBufferTextureFormat::RGBA8:
            return GL_RGBA8;
        case FrameBufferTextureFormat::RedInteger:
            return GL_R32I;
        }

        FLARE_CORE_ASSERT(false, "Unhanded frame buffer texture format");
        return 0;
    }

    static GLenum FrameBufferTextureFormatToOpenGLFormat(FrameBufferTextureFormat format)
    {
        switch (format)
        {
        case FrameBufferTextureFormat::RGB8:
            return GL_RGB;
        case FrameBufferTextureFormat::RGBA8:
            return GL_RGBA;
        case FrameBufferTextureFormat::RedInteger:
            return GL_RED_INTEGER;
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
        glDeleteFramebuffers(1, &m_Id);
        glDeleteTextures((int32_t)m_ColorAttachments.size(), m_ColorAttachments.data());

        m_Specifications.Width = width;
        m_Specifications.Height = height;

        Create();
    }

    void* OpenGLFrameBuffer::GetColorAttachmentRendererId(uint32_t attachmentIndex)
    {
        FLARE_CORE_ASSERT((size_t)attachmentIndex < m_ColorAttachments.size());
        return (void*)(size_t)(m_ColorAttachments[attachmentIndex]);
    }

    uint32_t OpenGLFrameBuffer::GetAttachmentsCount()
    {
        return (uint32_t)m_ColorAttachments.size();
    }

    void OpenGLFrameBuffer::ClearAttachment(uint32_t index, uint32_t value)
    {
        FLARE_CORE_ASSERT(index < m_ColorAttachments.size());
        glClearTexImage(m_ColorAttachments[index], 0, FrameBufferTextureFormatToOpenGLFormat(m_Specifications.Attachments[index].Format), GL_INT, &value);
    }

    void OpenGLFrameBuffer::ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, void* pixelOutput)
    {
        FLARE_CORE_ASSERT(attachmentIndex < m_ColorAttachments.size());
        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

        glReadPixels(x, y, 1, 1, FrameBufferTextureFormatToOpenGLFormat(m_Specifications.Attachments[attachmentIndex].Format), GL_INT, pixelOutput);
    }

    void OpenGLFrameBuffer::Blit(const Ref<FrameBuffer>& source, uint32_t destinationAttachment, uint32_t sourceAttachment)
    {
        Ref<OpenGLFrameBuffer> sourceFrameBuffer = As<OpenGLFrameBuffer>(source);
        FLARE_CORE_ASSERT(destinationAttachment < (uint32_t)m_ColorAttachments.size());
        FLARE_CORE_ASSERT(sourceAttachment < (uint32_t)sourceFrameBuffer->m_ColorAttachments.size());
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFrameBuffer->m_Id);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_Id);

        glDrawBuffer(GL_COLOR_ATTACHMENT0 + destinationAttachment);
        glReadBuffer(GL_COLOR_ATTACHMENT0 + sourceAttachment);

        glBlitFramebuffer(0, 0, 
            m_Specifications.Width, 
            m_Specifications.Height, 
            0, 0, 
            sourceFrameBuffer->m_Specifications.Width,
            sourceFrameBuffer->m_Specifications.Height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);

        GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers((int32_t)m_ColorAttachments.size(), drawBuffers);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        auto t = glGetError();
        if (t != GL_NO_ERROR)
            FLARE_CORE_INFO("{0}", t);
    }

    void OpenGLFrameBuffer::BindAttachmentTexture(uint32_t attachment, uint32_t slot)
    {
        FLARE_CORE_ASSERT(attachment < (uint32_t)m_ColorAttachments.size());
        glBindTextureUnit(slot, m_ColorAttachments[attachment]);
    }

    void OpenGLFrameBuffer::Create()
    {
        glCreateFramebuffers(1, &m_Id);
        glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
        glCreateTextures(GL_TEXTURE_2D, (int32_t)m_ColorAttachments.size(), m_ColorAttachments.data());

        for (size_t i = 0; i < m_ColorAttachments.size(); i++)
        {
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachments[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, 
                FrameBufferTextureForamtToOpenGLInternalFormat(m_Specifications.Attachments[i].Format),
                m_Specifications.Width, m_Specifications.Height, 0,
                FrameBufferTextureFormatToOpenGLFormat(m_Specifications.Attachments[i].Format),
                GL_UNSIGNED_BYTE, nullptr);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, (GLenum)(GL_COLOR_ATTACHMENT0 + i), GL_TEXTURE_2D, m_ColorAttachments[i], 0);
        }

        FLARE_CORE_ASSERT(m_ColorAttachments.size() <= 3);
        GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
        glDrawBuffers((int32_t)m_ColorAttachments.size(), drawBuffers);

        FLARE_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Frame buffer is incomplete");

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
