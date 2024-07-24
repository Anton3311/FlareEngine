#include "VulkanAllocation.h"

#include "FlareCore/Assert.h"

namespace Flare
{
	VulkanAllocation::~VulkanAllocation()
	{
		FLARE_CORE_ASSERT(Handle == VK_NULL_HANDLE);
	}
}
