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
		if (m_DebugMessenger)
			m_DestroyDebugMessenger(m_Instance, m_DebugMessenger, nullptr);

		ReleaseSwapChain();

		vkDestroyDevice(m_Device, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
	}

	void VulkanContext::Initialize()
	{
		std::vector<VkLayerProperties> supportedLayers = EnumerateAvailableLayers();
		std::vector<const char*> enabledLayers;
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

		auto addIfSupported = [&](const char* layerName)
		{
			if (std::find_if(
				supportedLayers.begin(),
				supportedLayers.end(),
				[&](const VkLayerProperties& a) -> bool { return std::strcmp(a.layerName, layerName); }) == supportedLayers.end())
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

		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

			FLARE_CORE_INFO("Physical device name: {}", properties.deviceName);
		}

		GetQueueFamilyProperties();

		std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE1_EXTENSION_NAME };
		CreateLogicalDevice(Span<const char*>::FromVector(enabledLayers), Span<const char*>::FromVector(deviceExtensions));
	}

	void VulkanContext::SwapBuffers()
	{
		glfwSwapBuffers(m_Window);
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
	}

	void VulkanContext::ReleaseSwapChain()
	{
		vkDestroySwapchainKHR(m_Device, m_SwapChain, nullptr);
		m_SwapChainImages.clear();

		m_SwapChain = VK_NULL_HANDLE;
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
