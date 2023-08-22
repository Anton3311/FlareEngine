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
		virtual void Bind() override;
		virtual void Unbind() override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual void* GetColorAttachmentRendererId(uint32_t attachmentIndex) override;

		virtual uint32_t GetAttachmentsCount() override;
		virtual void ClearAttachment(uint32_t index, uint32_t value) override;
		virtual void ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, void* pixelOutput) override;
		virtual void Blit(const Ref<FrameBuffer>& source, uint32_t destinationAttachment, uint32_t sourceAttachment) override;
		virtual void BindAttachmentTexture(uint32_t attachment, uint32_t slot = 0) override;

		virtual const FrameBufferSpecifications& GetSpecifications() const override { return m_Specifications; }
	private:
		void Create();
	private:
		FrameBufferSpecifications m_Specifications;

		uint32_t m_Id;
		std::vector<uint32_t> m_ColorAttachments;
	};
}