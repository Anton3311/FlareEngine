#pragma once

#include "Flare/Renderer/FrameBuffer.h"

namespace Flare
{
	class OpenGLFrameBuffer : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(const FrameBufferSpecifications& specifications);
		~OpenGLFrameBuffer();
	public:
		uint32_t GetAttachmentId(uint32_t attachmentIndex) const { return m_ColorAttachments[attachmentIndex]; }

		virtual void Bind() override;
		virtual void Unbind() override;

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual uint32_t GetAttachmentsCount() const override;
		virtual void ClearAttachment(uint32_t index, const void* value) override;
		virtual void ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, void* pixelOutput) override;
		virtual void Blit(const Ref<FrameBuffer>& source, uint32_t destinationAttachment, uint32_t sourceAttachment) override;
		virtual void BindAttachmentTexture(uint32_t attachment, uint32_t slot = 0) override;

		virtual void SetWriteMask(FrameBufferAttachmentsMask mask) override;
		virtual FrameBufferAttachmentsMask GetWriteMask() override;

		virtual const FrameBufferSpecifications& GetSpecifications() const override { return m_Specifications; }
	private:
		void Create();
		void AttachColorTexture(uint32_t index);
		void AttachDepthTexture(uint32_t index);
	private:
		FrameBufferSpecifications m_Specifications;

		uint32_t m_Id;
		std::vector<uint32_t> m_ColorAttachments;
		FrameBufferAttachmentsMask m_WriteMask;
	};
}