#pragma once

#include "FlareCore/Core.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace Flare
{
	struct FLARE_API VulkanAllocation
	{
		~VulkanAllocation();

		VmaAllocation Handle = VK_NULL_HANDLE;
		VmaAllocationInfo Info{};
	};
}
