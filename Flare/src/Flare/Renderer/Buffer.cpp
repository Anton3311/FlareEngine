#include "Buffer.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Flare/Platform/Vulkan/VulkanIndexBuffer.h"

namespace Flare
{
	Ref<VertexBuffer> VertexBuffer::Create(size_t size)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanVertexBuffer>(size);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(size_t size, GPUBufferUsage usage)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanVertexBuffer>(size, usage);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(size_t size, const void* data)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanVertexBuffer>(data, size);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(size_t size, const void* data, Ref<CommandBuffer> commandBuffer)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanVertexBuffer>(data, size, commandBuffer);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	size_t IndexBuffer::GetIndexFormatSize(IndexFormat format)
	{
		switch (format)
		{
		case IndexFormat::UInt16:
			return sizeof(uint16_t);
		case IndexFormat::UInt32:
			return sizeof(uint32_t);
		}

		FLARE_CORE_ASSERT(false);
		return 0;
	}

	Ref<IndexBuffer> IndexBuffer::Create(IndexFormat format, size_t size)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanIndexBuffer>(format, size);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(IndexFormat format, size_t size, GPUBufferUsage usage)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanIndexBuffer>(format, size, usage);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}
	
	Ref<IndexBuffer> IndexBuffer::Create(IndexBuffer::IndexFormat format, const MemorySpan& indices)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanIndexBuffer>(format, indices);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(IndexFormat format, const MemorySpan& indices, Ref<CommandBuffer> commandBuffer)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			return CreateRef<VulkanIndexBuffer>(format, indices, commandBuffer);
		}

		FLARE_CORE_ASSERT(false);
		return nullptr;
	}
}