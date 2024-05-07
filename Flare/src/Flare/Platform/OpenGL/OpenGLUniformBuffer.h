#pragma once

#include "Flare/Renderer/UniformBuffer.h"

namespace Flare
{
	class OpenGLUniformBuffer : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(size_t size, uint32_t binding);
		virtual ~OpenGLUniformBuffer();

		void SetData(const void* data, size_t size, size_t offset) override;
		void Bind() const override;
		size_t GetSize() const override;
	private:
		uint32_t m_Id = UINT32_MAX;
		uint32_t m_Binding = UINT32_MAX;
		size_t m_Size = 0;
	};
}