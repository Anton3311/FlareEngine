#pragma once

#include "Flare/Renderer/FrameBuffer.h"

namespace Flare
{
	class FLARE_API OpenGLFrameBuffer : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(const FrameBufferSpecifications& specifications);
		~OpenGLFrameBuffer();
	public:
		inline uint32_t GetAttachmentId(uint32_t attachmentIndex) const { return m_ColorAttachments[attachmentIndex]; }
		inline uint32_t GetId() const { return m_Id; }

		void Bind();
		void Unbind();

		virtual void Resize(uint32_t width, uint32_t height) override;

		virtual uint32_t GetAttachmentsCount() const override;
		virtual uint32_t GetColorAttachmentsCount() const override;
		virtual std::optional<uint32_t> GetDepthAttachmentIndex() const override;

		virtual void ClearAttachment(uint32_t index, const void* value) override;
		virtual void ReadPixel(uint32_t attachmentIndex, uint32_t x, uint32_t y, void* pixelOutput) override;
		virtual void BindAttachmentTexture(uint32_t attachment, uint32_t slot = 0) override;

		virtual const FrameBufferSpecifications& GetSpecifications() const override { return m_Specifications; }
	private:
		void Create();
		void AttachColorTexture(uint32_t index);
		void AttachDepthTexture(uint32_t index);
	private:
		FrameBufferSpecifications m_Specifications;

		uint32_t m_Id;
		std::vector<uint32_t> m_ColorAttachments;
		std::optional<uint32_t> m_DepthAttachmentIndex;
	};
}