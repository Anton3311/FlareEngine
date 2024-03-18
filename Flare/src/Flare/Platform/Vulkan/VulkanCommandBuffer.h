#pragma once

#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class FLARE_API VulkanCommandBuffer
	{
	public:
		VulkanCommandBuffer(VkCommandBuffer commandBuffer);

		void Reset();

		void Begin();
		void End();

		void BeginRenderPass(const Ref<VulkanRenderPass>& renderPass, const Ref<VulkanFrameBuffer>& frameBuffer);
		void EndRenderPass();

		void ClearImage(VkImage image, const glm::vec4& clearColor, VkImageLayout oldLayout, VkImageLayout newLayout);

		VkCommandBuffer GetHandle() const { return m_CommandBuffer; }
	private:
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
	};
}
