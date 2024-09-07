#include "VulkanBuffer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	static VulkanBuffer::PipelineDependency CreatePipelineDependency(GPUBufferUsage usage)
	{
		VulkanBuffer::PipelineDependency dependency{};

		if (HAS_BIT(usage, GPUBufferUsage::IndexBuffer))
		{
			dependency.DependentStages |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
			dependency.AccessFlags |= VK_ACCESS_INDEX_READ_BIT;
		}

		if (HAS_BIT(usage, GPUBufferUsage::IndirectDrawBuffer))
		{
			dependency.DependentStages |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
			dependency.AccessFlags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
		}

		if (HAS_BIT(usage, GPUBufferUsage::Readback))
		{
			dependency.DependentStages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			dependency.AccessFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
		}

		if (HAS_BIT(usage, GPUBufferUsage::StorageBuffer))
		{
			dependency.DependentStages |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
				| VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			dependency.AccessFlags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		}

		if (HAS_BIT(usage, GPUBufferUsage::UniformBuffer))
		{
			dependency.DependentStages |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
				| VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			dependency.AccessFlags |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		}

		if (HAS_BIT(usage, GPUBufferUsage::VertexBuffer))
		{
			dependency.DependentStages |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			dependency.AccessFlags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
		}

		return dependency;
	}

	static VkBufferUsageFlags BufferUsageToVulkanBufferUsage(GPUBufferUsage usage)
	{
		FLARE_CORE_ASSERT(usage != GPUBufferUsage::None);

		VkBufferUsageFlags flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		if (HAS_BIT(usage, GPUBufferUsage::IndexBuffer))
			flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		if (HAS_BIT(usage, GPUBufferUsage::IndirectDrawBuffer))
			flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		if (HAS_BIT(usage, GPUBufferUsage::Readback))
			flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		if (HAS_BIT(usage, GPUBufferUsage::StorageBuffer))
			flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		if (HAS_BIT(usage, GPUBufferUsage::UniformBuffer))
			flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		if (HAS_BIT(usage, GPUBufferUsage::VertexBuffer))
			flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		return flags;
	}

	VulkanBuffer::VulkanBuffer(const GPUBufferSpecifications& specifications)
		: m_Specifications(specifications)
	{
		EnsureAllocated();
	}

	VulkanBuffer::~VulkanBuffer()
	{
		Release();
	}

	void VulkanBuffer::SetData(MemorySpan data, size_t offset)
	{
		FLARE_PROFILE_FUNCTION();
		if (data.GetSize() == 0)
			return;

		if (m_Specifications.Size == 0)
			m_Specifications.Size = data.GetSize();

		FLARE_CORE_ASSERT(data.GetSize() <= m_Specifications.Size);

		EnsureAllocated();

		if (m_Specifications.MemoryType == GPUBufferMemoryType::Dynamic)
		{
			FLARE_CORE_ASSERT(m_Mapped);

			std::memcpy((uint8_t*)m_Mapped + offset, data.GetBuffer(), data.GetSize());
		}
		else
		{
			VulkanStagingBufferPool& stagingBufferPool = VulkanContext::GetInstance().GetStagingBufferPool();

			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();
			VulkanStagingBuffer stagingBuffer = FillStagingBuffer(data);

			commandBuffer->CopyBuffer(stagingBuffer.Buffer, m_Buffer, data.GetSize(), stagingBuffer.Offset, offset);
			VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);

			stagingBufferPool.ReleaseStagingBuffer(stagingBuffer);
		}
	}

	void VulkanBuffer::SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Specifications.MemoryType == GPUBufferMemoryType::Static);

		if (data.GetSize() == 0)
			return;

		if (m_Specifications.Size == 0)
			m_Specifications.Size = data.GetSize();

		FLARE_CORE_ASSERT(data.GetSize() + offset <= m_Specifications.Size);
		FLARE_CORE_ASSERT(data.GetSize() <= m_Specifications.Size);

		EnsureAllocated();

		VulkanStagingBuffer stagingBuffer = FillStagingBuffer(data);

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = commandBuffer.As<VulkanCommandBuffer>();

		VkBufferMemoryBarrier barriers[2] = {};
		barriers[0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[0].buffer = m_Buffer;
		barriers[0].offset = (VkDeviceSize)offset;
		barriers[0].size = m_Specifications.Size - offset;
		barriers[0].pNext = nullptr;
		barriers[0].srcAccessMask = m_PipelineDependency.AccessFlags;
		barriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		barriers[1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barriers[1].buffer = m_Buffer;
		barriers[1].offset = (VkDeviceSize)offset;
		barriers[1].size = m_Specifications.Size - offset;
		barriers[1].pNext = nullptr;
		barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barriers[1].dstAccessMask = m_PipelineDependency.AccessFlags;
		barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		vulkanCommandBuffer->AddBufferBarrier(Span(&barriers[0], 1), m_PipelineDependency.DependentStages, VK_PIPELINE_STAGE_TRANSFER_BIT);
		vulkanCommandBuffer->CopyBuffer(stagingBuffer.Buffer, m_Buffer, data.GetSize(), stagingBuffer.Offset, offset);
		vulkanCommandBuffer->AddBufferBarrier(Span(&barriers[1], 1), VK_PIPELINE_STAGE_TRANSFER_BIT, m_PipelineDependency.DependentStages);
	}

	void VulkanBuffer::ReadData(size_t readOffset, void* outBuffer)
	{
	}

	void VulkanBuffer::Resize(size_t newSize)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(newSize > 0);

		Release();

		m_Specifications.Size = newSize;

		Create();

		if (m_DebugName.size() > 0)
			UpdateDebugName();
	}

	const GPUBufferSpecifications& VulkanBuffer::GetSpecifications() const
	{
		return m_Specifications;
	}

	void VulkanBuffer::EnsureAllocated()
	{
		if (m_Buffer)
			return;
		
		Create();
	}

	void VulkanBuffer::SetDebugName(std::string_view name)
	{
		m_DebugName = name;
		UpdateDebugName();
	}

	const std::string& VulkanBuffer::GetDebugName() const
	{
		return m_DebugName;
	}

	void VulkanBuffer::Create()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Specifications.Size > 0);
		FLARE_CORE_ASSERT(m_Buffer == VK_NULL_HANDLE);

		VkBufferCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		info.flags = 0;
		info.pNext = nullptr;
		info.pQueueFamilyIndices = nullptr;
		info.queueFamilyIndexCount = 0;
		info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		info.size = (VkDeviceSize)m_Specifications.Size;
		info.usage = BufferUsageToVulkanBufferUsage(m_Specifications.Usage);

		VmaAllocationCreateInfo allocation{};

		switch (m_Specifications.MemoryType)
		{
		case GPUBufferMemoryType::Static:
			allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			break;
		case GPUBufferMemoryType::Dynamic:
			allocation.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			allocation.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
			break;
		}

		VK_CHECK_RESULT(vmaCreateBuffer(VulkanContext::GetInstance().GetMemoryAllocator(), &info, &allocation, &m_Buffer, &m_Allocation.Handle, &m_Allocation.Info));

		if (m_Specifications.MemoryType == GPUBufferMemoryType::Dynamic)
		{
			VK_CHECK_RESULT(vmaMapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_Allocation.Handle, &m_Mapped));
		}
	}

	void VulkanBuffer::Release()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(VulkanContext::GetInstance().IsValid());

		VkDevice device = VulkanContext::GetInstance().GetDevice();

		if (m_Mapped)
		{
			vmaUnmapMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_Allocation.Handle);
			m_Mapped = nullptr;
		}

		vmaFreeMemory(VulkanContext::GetInstance().GetMemoryAllocator(), m_Allocation.Handle);
		vkDestroyBuffer(device, m_Buffer, nullptr);

		m_Buffer = VK_NULL_HANDLE;
		m_Allocation = {};
	}

	void VulkanBuffer::UpdateDebugName()
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Buffer == VK_NULL_HANDLE)
			return;

		VulkanContext::GetInstance().SetDebugName(VK_OBJECT_TYPE_BUFFER, (uint64_t)m_Buffer, m_DebugName.c_str());
	}

	VulkanStagingBuffer VulkanBuffer::FillStagingBuffer(MemorySpan data)
	{
		FLARE_PROFILE_FUNCTION();
		VulkanStagingBuffer stagingBuffer = VulkanContext::GetInstance().GetStagingBufferPool().AllocateStagingBuffer(data.GetSize());

		std::memcpy(stagingBuffer.Mapped, data.GetBuffer(), data.GetSize());

		return stagingBuffer;
	}
}
