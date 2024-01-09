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
		size_t m_BufferSize;
	};

	class OpenGLIndexBuffer : public IndexBuffer
	{
	public:
		OpenGLIndexBuffer(IndexFormat format, size_t size);
		OpenGLIndexBuffer(IndexFormat format, const MemorySpan& indices);
		~OpenGLIndexBuffer();
	public:
		virtual void Bind() override;
		virtual void SetData(const MemorySpan& indices, size_t offset = 0) override;
		virtual size_t GetCount() const override;
		virtual IndexFormat GetIndexFormat() const override;
	private:
		uint32_t m_Id;
		size_t m_SizeInBytes;
		IndexFormat m_Format;
	};
}