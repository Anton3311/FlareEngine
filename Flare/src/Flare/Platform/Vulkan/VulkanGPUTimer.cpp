#include "VulkanGPUTimer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	VulkanGPUTimer::VulkanGPUTimer()
	{
		VkQueryPoolCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		info.queryCount = 2;
		info.queryType = VK_QUERY_TYPE_TIMESTAMP;
		
		VK_CHECK_RESULT(vkCreateQueryPool(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_Pool));
	}

	VulkanGPUTimer::~VulkanGPUTimer()
	{
		FLARE_CORE_ASSERT(GraphicsContext::IsInitialized());
		vkDestroyQueryPool(VulkanContext::GetInstance().GetDevice(), m_Pool, nullptr);
	}

	std::optional<float> VulkanGPUTimer::GetElapsedTime()
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(VulkanContext::GetInstance().GetPhysicalDevice(), &properties);

		uint64_t timestamps[4] = { 0 };
		VkResult result = vkGetQueryPoolResults(
			VulkanContext::GetInstance().GetDevice(),
			m_Pool, 0, 2, sizeof(timestamps),
			&timestamps, 2 * sizeof(uint64_t),
			VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

		if (result == VK_NOT_READY)
			return {};

		VK_CHECK_RESULT(result);

		if (timestamps[1] != 1 || timestamps[3] != 1)
			return {};

		uint64_t time = timestamps[2] - timestamps[0];
		double milliseconds = time * properties.limits.timestampPeriod / 1000000.0;

		return (float)milliseconds;
	}
}
