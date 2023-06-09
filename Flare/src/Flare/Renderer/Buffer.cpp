#include "Buffer.h"

#include <Flare/Renderer/RendererAPI.h>

#include <Flare/Platform/OpenGL/OpenGLBuffer.h>

namespace Flare
{
	Ref<VertexBuffer> VertexBuffer::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLVertexBuffer>();
		}
	}

	Ref<VertexBuffer> VertexBuffer::Create(size_t size, const void* data)
	{
		Ref<VertexBuffer> vertexBuffer = nullptr;

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			vertexBuffer = CreateRef<OpenGLVertexBuffer>();
			break;
		}

		vertexBuffer->SetData(data, size);
		return vertexBuffer;
	}
	
	Ref<IndexBuffer> IndexBuffer::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLIndexBuffer>();
		}
	}
	
	Ref<IndexBuffer> IndexBuffer::Create(const void* data, size_t size)
	{
		Ref<IndexBuffer> indexBuffer = nullptr;

		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			indexBuffer = CreateRef<OpenGLIndexBuffer>();
			break;
		}

		indexBuffer->SetData(data, size);
		return indexBuffer;
	}
}