#include "VulkanIndexBuffer.h"

namespace Flare
{
	VulkanIndexBuffer::VulkanIndexBuffer(IndexFormat format, size_t count)
		: m_Format(format), m_Count(count), m_Buffer(
			GPUBufferUsage::Dynamic,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VulkanBuffer::PipelineDependecy(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT),
			GetIndexFormatSize(format) * count)
	{
	}

	VulkanIndexBuffer::VulkanIndexBuffer(IndexFormat format, size_t count, GPUBufferUsage usage)
		: m_Format(format), m_Count(count), m_Buffer(
			usage,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VulkanBuffer::PipelineDependecy(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT),
			GetIndexFormatSize(format)* count)
	{
	}

	VulkanIndexBuffer::VulkanIndexBuffer(IndexFormat format, const MemorySpan& indices)
		: m_Format(format),
		m_Buffer(
			GPUBufferUsage::Static,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VulkanBuffer::PipelineDependecy(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT),
			indices.GetSize())
	{
		m_Buffer.SetData(indices.GetBuffer(), indices.GetSize(), 0);
		m_Count = indices.GetSize() / IndexBuffer::GetIndexFormatSize(m_Format);
	}

	VulkanIndexBuffer::VulkanIndexBuffer(IndexFormat format, const MemorySpan& indices, Ref<CommandBuffer> commandBuffer)
		: m_Format(format),
		m_Buffer(
			GPUBufferUsage::Static,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VulkanBuffer::PipelineDependecy(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_INDEX_READ_BIT),
			indices.GetSize())
	{
		m_Buffer.SetData(indices, 0, commandBuffer);
		m_Count = indices.GetSize() / IndexBuffer::GetIndexFormatSize(m_Format);
	}

	void Flare::VulkanIndexBuffer::SetData(const MemorySpan& indices, size_t offset)
	{
		m_Buffer.SetData(indices.GetBuffer(), indices.GetSize(), offset);
	}

	void VulkanIndexBuffer::SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer)
	{
		m_Buffer.SetData(data, offset, commandBuffer);
	}

	size_t Flare::VulkanIndexBuffer::GetCount() const
	{
		return m_Count;
	}

	IndexBuffer::IndexFormat Flare::VulkanIndexBuffer::GetIndexFormat() const
	{
		return m_Format;
	}

	void VulkanIndexBuffer::SetDebugName(std::string_view debugName)
	{
		m_Buffer.SetDebugName(debugName);
	}

	const std::string& VulkanIndexBuffer::GetDebugName() const
	{
		return m_Buffer.GetDebugName();
	}
}
