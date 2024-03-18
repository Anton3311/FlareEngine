#include "VulkanIndexBuffer.h"

namespace Flare
{
	void Flare::VulkanIndexBuffer::Bind()
	{
	}

	void Flare::VulkanIndexBuffer::SetData(const MemorySpan& indices, size_t offset)
	{
	}

	size_t Flare::VulkanIndexBuffer::GetCount() const
	{
		return 0;
	}

	IndexBuffer::IndexFormat Flare::VulkanIndexBuffer::GetIndexFormat() const
	{
		return IndexBuffer::IndexFormat::UInt32;
	}
}
