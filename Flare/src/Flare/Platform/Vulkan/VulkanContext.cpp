#include "VulkanContext.h"

namespace Flare
{
	static VkBool32 VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			return VK_FALSE;

		const char* messageType = "";
		switch (messageTypes)
		{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			messageType = "General";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			messageType = "Performance";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			messageType = "Validation";
			break;
		}

		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			FLARE_CORE_INFO("Validation layers[Info]: {} {}", messageType, pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			FLARE_CORE_ERROR("Validation layers[Error]: {} {}", messageType, pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			FLARE_CORE_WARN("Validation layers[Warning]: {} {}", messageType, pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			FLARE_CORE_TRACE("Validation layers[Verbose]: {} {}", messageType, pCallbackData->pMessage);
			break;
		}

		return VK_FALSE;
	}


	VulkanContext::VulkanContext(GLFWwindow* window)
		: m_Window(window)
	{
	}

	VulkanContext::~VulkanContext()
	{
		WaitForDevice();

		if (m_DebugMessenger)
			m_DestroyDebugMessenger(m_Instance, m_DebugMessenger, nullptr);

		ReleaseSwapChain();

		vkFreeCommandBuffers(m_Device, m_CommandBufferPool, 1, &m_CommandBuffer);
		vkDestroyCommandPool(m_Device, m_CommandBufferPool, nullptr);

		vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
		vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
		vkDestroyFence(m_Device, m_FrameFence, nullptr);

		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

		vkDestroyDevice(m_Device, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanContext::Initialize()
	{
		std::vector<VkLayerProperties> supportedLayers = EnumerateAvailableLayers();
		std::vector<const char*> enabledLayers;
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

		for (auto l : supportedLayers)
		{
			FLARE_CORE_INFO(l.layerName);
		}

		auto addIfSupported = [&](const char* layerName)
		{
			if (std::find_if(
				supportedLayers.begin(),
				supportedLayers.end(),
				[&](const VkLayerProperties& a) -> bool { return std::strcmp(a.layerName, layerName) == 0; }) != supportedLayers.end())
			{
				enabledLayers.push_back(layerName);
			}
		};

		addIfSupported(validationLayerName);

		CreateInstance(Span<const char*>::FromVector(enabledLayers));

		if (m_DebugEnabled)
			CreateDebugMessenger();

		CreateSurface();
		ChoosePhysicalDevice();
		GetQueueFamilyProperties();

		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

			FLARE_CORE_INFO("Physical device name: {}", properties.deviceName);
		}

		std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME };
		CreateLogicalDevice(Span<const char*>::FromVector(enabledLayers), Span<const char*>::FromVector(deviceExtensions));

		CreateSwapChain();

		CreateSyncObjects();
		CreateCommandBufferPool();
		m_CommandBuffer = CreateCommandBuffer();
	}

	void VulkanContext::Present()
	{
		VK_CHECK_RESULT(vkWaitForFences(m_Device, 1, &m_FrameFence, VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetFences(m_Device, 1, &m_FrameFence));

		uint32_t frameIndex = 0;
		VkResult acquireResult = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphore, VK_NULL_HANDLE, &frameIndex);

		if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapChain();
			return;
		}
		else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
		{
			FLARE_CORE_ERROR("Failed to acquire swap chain image");
			return;
		}

		vkResetCommandBuffer(m_CommandBuffer, 0);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);

		ClearImage(m_CommandBuffer, m_SwapChainImages[frameIndex], glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		vkEndCommandBuffer(m_CommandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;

		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_RenderFinishedSemaphore;

		VK_CHECK_RESULT(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_FrameFence));

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &m_RenderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_SwapChain;
		presentInfo.pImageIndices = &frameIndex;

		VkResult presentResult = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
			RecreateSwapChain();
		else if (presentResult != VK_SUCCESS)
			FLARE_CORE_ERROR("Failed to present");

		glfwSwapBuffers(m_Window);
	}

	void VulkanContext::ClearImage(VkCommandBuffer commandBuffer, VkImage image, const glm::vec4& clearColor, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageSubresourceRange range{};
		range.baseArrayLayer = 0;
		range.layerCount = 1;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange = range;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}

		VkClearColorValue value;
		value.float32[0] = clearColor.r;
		value.float32[1] = clearColor.g;
		value.float32[2] = clearColor.b;
		value.float32[3] = clearColor.a;

		vkCmdClearColorImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &range);

		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange = range;

			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}
	}

	void VulkanContext::WaitForDevice()
	{
		VK_CHECK_RESULT(vkDeviceWaitIdle(m_Device));
	}

	void VulkanContext::CreateInstance(const Span<const char*>& enabledLayers)
	{
		std::vector<const char*> instanceExtensions;

		{
			uint32_t count = 0;
			const char** extesions = glfwGetRequiredInstanceExtensions(&count);

			for (uint32_t i = 0; i < count; i++)
				instanceExtensions.push_back(extesions[i]);
		}

		if (m_DebugEnabled)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		VkApplicationInfo applicationInfo{};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.apiVersion = VK_API_VERSION_1_3;
		applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.pApplicationName = "Flare";
		applicationInfo.pEngineName = "Flare";

		VkInstanceCreateInfo instanceInfo{};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceInfo.ppEnabledExtensionNames = instanceExtensions.data();
		instanceInfo.enabledLayerCount = (uint32_t)enabledLayers.GetSize();
		instanceInfo.ppEnabledLayerNames = enabledLayers.GetData();

		VK_CHECK_RESULT(vkCreateInstance(&instanceInfo, nullptr, &m_Instance));
	}

	void VulkanContext::CreateDebugMessenger()
	{
		FLARE_CORE_ASSERT(m_DebugEnabled);

		m_CreateDebugMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
		m_DestroyDebugMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));

		FLARE_CORE_ASSERT(m_CreateDebugMessenger && m_DestroyDebugMessenger);

		VkDebugUtilsMessengerCreateInfoEXT info{};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		info.pNext = nullptr;
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		info.flags = 0;
		info.pfnUserCallback = VulkanDebugCallback;

		VK_CHECK_RESULT(m_CreateDebugMessenger(m_Instance, &info, nullptr, &m_DebugMessenger));
	}

	void VulkanContext::CreateSurface()
	{
		VK_CHECK_RESULT(glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface));
	}

	void VulkanContext::ChoosePhysicalDevice()
	{
		uint32_t physicalDevicesCount;
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, nullptr));

		std::vector<VkPhysicalDevice> devices(physicalDevicesCount);
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, devices.data()));

		for (VkPhysicalDevice device : devices)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);

			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				m_PhysicalDevice = device;
			}
		}

		FLARE_CORE_ASSERT(m_PhysicalDevice);
	}

	void VulkanContext::GetQueueFamilyProperties()
	{
		uint32_t count;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, nullptr);

		std::vector<VkQueueFamilyProperties> properties(count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, properties.data());

		for (uint32_t i = 0; i < count; i++)
		{
			if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				m_GraphicsQueueFamilyIndex = i;

			VkBool32 supported = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &supported);

			if (supported)
				m_PresentQueueFamilyIndex = i;
		}

		FLARE_CORE_ASSERT(m_GraphicsQueueFamilyIndex.has_value() && m_PresentQueueFamilyIndex.has_value());
	}

	void VulkanContext::CreateLogicalDevice(const Span<const char*>& enabledLayers, const Span<const char*>& enabledExtensions)
	{
		float priority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> createInfos;

		{
			VkDeviceQueueCreateInfo& queueCreateInfo = createInfos.emplace_back();
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = *m_GraphicsQueueFamilyIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;

			VkDeviceQueueCreateInfo& presentationQueueCreateInfo = createInfos.emplace_back();
			presentationQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			presentationQueueCreateInfo.queueFamilyIndex = *m_PresentQueueFamilyIndex;
			presentationQueueCreateInfo.queueCount = 1;
			presentationQueueCreateInfo.pQueuePriorities = &priority;
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};

		VkPhysicalDeviceSynchronization2Features synchronization2{};
		synchronization2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		synchronization2.synchronization2 = true;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = createInfos.data();
		deviceCreateInfo.queueCreateInfoCount = (uint32_t)createInfos.size();
		deviceCreateInfo.pNext = &synchronization2;

		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

		deviceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.GetSize();
		deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.GetData();

		deviceCreateInfo.enabledLayerCount = (uint32_t)enabledLayers.GetSize();
		deviceCreateInfo.ppEnabledLayerNames = enabledLayers.GetData();

		VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device));

		vkGetDeviceQueue(m_Device, *m_GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, *m_PresentQueueFamilyIndex, 0, &m_PresentQueue);
	}

	void VulkanContext::CreateCommandBufferPool()
	{
		VkCommandPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		createInfo.queueFamilyIndex = *m_GraphicsQueueFamilyIndex;

		VK_CHECK_RESULT(vkCreateCommandPool(m_Device, &createInfo, nullptr, &m_CommandBufferPool));
	}

	VkCommandBuffer VulkanContext::CreateCommandBuffer()
	{
		VkCommandBufferAllocateInfo allocation{};
		allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocation.commandBufferCount = 1;
		allocation.commandPool = m_CommandBufferPool;
		allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer commandBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_Device, &allocation, &commandBuffer));
		return commandBuffer;
	}

	void VulkanContext::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VK_CHECK_RESULT(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore));
		VK_CHECK_RESULT(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore));
		VK_CHECK_RESULT(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_FrameFence));
	}

	void VulkanContext::CreateSwapChain()
	{
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &surfaceCapabilities));

		uint32_t formatsCount;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatsCount, nullptr));

		std::vector<VkSurfaceFormatKHR> formats(formatsCount);
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatsCount, formats.data()));

		uint32_t presentModeCount;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr));

		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, presentModes.data()));

		uint32_t formatIndex = ChooseSwapChainFormat(formats);
		VkPresentModeKHR presentMode = ChoosePrensentMode(presentModes);
		VkExtent2D swapChainExtent = GetSwapChainExtent(surfaceCapabilities);

		m_SwapChainExtent.x = swapChainExtent.width;
		m_SwapChainExtent.y = swapChainExtent.height;

		uint32_t imageCount = glm::min(surfaceCapabilities.maxImageCount, surfaceCapabilities.minImageCount + 1);

		m_SwapChainImageFormat = formats[formatIndex].format;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = formats[formatIndex].format;
		createInfo.imageColorSpace = formats[formatIndex].colorSpace;
		createInfo.imageExtent = swapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		uint32_t queueFamilies[] = { *m_GraphicsQueueFamilyIndex, *m_PresentQueueFamilyIndex };

		if (*m_GraphicsQueueFamilyIndex != *m_PresentQueueFamilyIndex)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.pQueueFamilyIndices = queueFamilies;
			createInfo.queueFamilyIndexCount = 2;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.pQueueFamilyIndices = nullptr;
			createInfo.queueFamilyIndexCount = 0;
		}

		createInfo.preTransform = surfaceCapabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = true;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VK_CHECK_RESULT(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_SwapChain));

		uint32_t swapChainImageCount = 0;
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &swapChainImageCount, nullptr));

		m_SwapChainImages.resize(swapChainImageCount);
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_Device, m_SwapChain, &swapChainImageCount, m_SwapChainImages.data()));

		CreateSwapChainImageViews();
	}

	void VulkanContext::RecreateSwapChain()
	{
		ReleaseSwapChain();
		CreateSwapChain();
	}

	void VulkanContext::ReleaseSwapChain()
	{
		for (VkImageView view : m_SwapChainImageViews)
			vkDestroyImageView(m_Device, view, nullptr);
		m_SwapChainImageViews.clear();

		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		m_SwapChainImages.clear();

		m_SwapChain = VK_NULL_HANDLE;
	}

	void VulkanContext::CreateSwapChainImageViews()
	{
		m_SwapChainImageViews.resize(m_SwapChainImages.size());
		for (size_t i = 0; i < m_SwapChainImages.size(); i++)
		{
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.format = m_SwapChainImageFormat;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.image = m_SwapChainImages[i];

			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(m_Device, &imageViewCreateInfo, nullptr, &m_SwapChainImageViews[i]));
		}
	}

	uint32_t VulkanContext::ChooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		for (uint32_t i = 0; i < (uint32_t)formats.size(); i++)
		{
			if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB && formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
				return i;
		}

		return 0;
	}

	VkExtent2D VulkanContext::GetSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;

		int32_t width;
		int32_t height;
		glfwGetFramebufferSize(m_Window, &width, &height);

		VkExtent2D extent = { (uint32_t)width, (uint32_t)height };

		extent.width = glm::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = glm::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return extent;
	}

	VkPresentModeKHR VulkanContext::ChoosePrensentMode(const std::vector<VkPresentModeKHR>& modes)
	{
		for (auto mode : modes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	std::vector<VkLayerProperties> VulkanContext::EnumerateAvailableLayers()
	{
		uint32_t supportedLayersCount = 0;
		std::vector<VkLayerProperties> supportedLayers;

		VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&supportedLayersCount, nullptr));

		supportedLayers.resize(supportedLayersCount);

		VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&supportedLayersCount, supportedLayers.data()));

		return supportedLayers;
	}
}
