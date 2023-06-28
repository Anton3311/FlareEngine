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

		virtual const FrameBufferSpecifications& GetSpecifications() const override { return m_Specifications; }
	private:
		void Create();
	private:
		FrameBufferSpecifications m_Specifications;

		uint32_t m_Id;
		std::vector<uint32_t> m_ColorAttachments;
	};
}