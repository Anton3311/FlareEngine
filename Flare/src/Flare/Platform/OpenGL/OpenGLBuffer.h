#pragma once

#include "Flare/Renderer/Buffer.h"

#include <glad/glad.h>

namespace Flare
{
	class OpenGLVertexBuffer : public VertexBuffer
	{
	public:
		OpenGLVertexBuffer(size_t size);
		OpenGLVertexBuffer(size_t size, const void* data);
		~OpenGLVertexBuffer();
	public:
		virtual void Bind() override;
		virtual void SetData(const void* data, size_t size, size_t offset = 0) override;

		virtual const BufferLayout& GetLayout() const override { return m_Layout; }
		virtual void SetLayout(const BufferLayout& layout) override { m_Layout = layout; }
	private:
		uint32_t m_Id;
		BufferLayout m_Layout;
	};

	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(size_t count);
		OpenGLIndexBuffer(size_t count, const void* data);
		~OpenGLIndexBuffer();
	public:
		virtual void Bind() override;
		virtual void SetData(const void* indices, size_t count, size_t offset = 0) override;
		virtual size_t GetCount() const override { return m_Count; }
	private:
		uint32_t m_Id;
		size_t m_Count;

	};
}