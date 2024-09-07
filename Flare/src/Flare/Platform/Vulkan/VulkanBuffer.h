#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/Buffer.h"
#include "Flare/Platform/Vulkan/VulkanAllocation.h"
#include "Flare/Platform/Vulkan/VulkanStagingBufferPool.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class CommandBuffer;
	class FLARE_API VulkanBuffer : public GPUBuffer
	{
	public:
		struct PipelineDependency
		{
			PipelineDependency() = default;
			PipelineDependency(VkPipelineStageFlags stages, VkAccessFlags accessFlags)
				: DependentStages(stages), AccessFlags(accessFlags) {}

			VkPipelineStageFlags DependentStages = VK_PIPELINE_STAGE_NONE;
			VkAccessFlags AccessFlags = VK_ACCESS_NONE;
		};

		VulkanBuffer(const GPUBufferSpecifications& specifications);
		~VulkanBuffer();

		void SetData(MemorySpan data, size_t offset) override;
		void SetData(MemorySpan data, size_t offset, Ref<CommandBuffer> commandBuffer) override;
		void ReadData(size_t readOffset, void* outBuffer) override;
		void Resize(size_t newSize) override;
		const GPUBufferSpecifications& GetSpecifications() const override;

		void SetDebugName(std::string_view debugName) override;
		const std::string& GetDebugName() const override;

		inline VkBuffer GetBufferHandle() const { return m_Buffer; }
	private:
		void Create();
		void Release();
		void UpdateDebugName();
		VulkanStagingBuffer FillStagingBuffer(MemorySpan data);
	protected:
		std::string m_DebugName;

		GPUBufferSpecifications m_Specifications;

		PipelineDependency m_PipelineDependency;

		void* m_Mapped = nullptr;

		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VulkanAllocation m_Allocation;
	};
}
