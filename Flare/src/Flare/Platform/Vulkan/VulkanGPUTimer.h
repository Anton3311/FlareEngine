#pragma once

#include <vulkan/vulkan.h>

#include <optional>

namespace Flare
{
	class VulkanGPUTimer
	{
	public:
		VulkanGPUTimer();
		~VulkanGPUTimer();

		std::optional<float> GetResult() const;

		VkQueryPool GetPoolHandle() const { return m_Pool; }
	private:
		VkQueryPool m_Pool = VK_NULL_HANDLE;
	};
}
