#include "Buffer.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/OpenGL/OpenGLBuffer.h"

namespace Flare
{
	Ref<VertexBuffer> VertexBuffer::Create(size_t size)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLVertexBuffer>(size);
		}
	}

	Ref<VertexBuffer> VertexBuffer::Create(size_t size, const void* data)
	{
		Ref<VertexBuffer> vertexBuffer = nullptr;

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			vertexBuffer = CreateRef<OpenGLVertexBuffer>(size, data);
			break;
		}

		vertexBuffer->SetData(data, size);
		return vertexBuffer;
	}
	
	Ref<IndexBuffer> IndexBuffer::Create(size_t count)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLIndexBuffer>(count);
		}
	}
	
	Ref<IndexBuffer> IndexBuffer::Create(size_t count, const void* data)
	{
		Ref<IndexBuffer> indexBuffer = nullptr;

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			indexBuffer = CreateRef<OpenGLIndexBuffer>(count, data);
			break;
		}
		return indexBuffer;
	}
}