#include "VulkanSwapchain.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"

namespace Flare
{
	VulkanSwapchain::VulkanSwapchain(VkSurfaceKHR surface)
		: m_Surface(surface)
	{
		FLARE_PROFILE_FUNCTION();
	}

	VulkanSwapchain::~VulkanSwapchain()
	{
		FLARE_PROFILE_FUNCTION();
		Release();
	}

	void VulkanSwapchain::Initialize()
	{
		std::vector<VkSurfaceFormatKHR> surfaceFormats = GetSurfaceFormats();
		uint32_t formatIndex = ChooseImageFormat(surfaceFormats);

		m_ImageFormat = surfaceFormats[formatIndex].format;
		m_ColorSpace = surfaceFormats[formatIndex].colorSpace;
	}

	void VulkanSwapchain::AcquireNextImage()
	{
		FLARE_PROFILE_FUNCTION();

		m_CurrentImageAvailableSemaphore = (m_CurrentImageAvailableSemaphore + 1) % (uint32_t)m_FrameData.size();

		VkDevice device = VulkanContext::GetInstance().GetDevice();

		VkResult acquireResult = vkAcquireNextImageKHR(
			device,
			m_Swapchain,
			UINT64_MAX,
			m_ImageAvailableSemaphores[m_CurrentImageAvailableSemaphore],
			VK_NULL_HANDLE,
			&m_CurrentFrameInFlight);

		if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			Recreate();
			return;
		}
		else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
		{
			FLARE_CORE_ERROR("Failed to acquire swap chain image");
			return;
		}
	}

	void VulkanSwapchain::Present(Span<const VkSemaphore> waitSemaphores, glm::uvec2 windowSize)
	{
		FLARE_PROFILE_FUNCTION();

		m_WindowSize = windowSize;

		std::vector<VkSemaphore> semaphores(waitSemaphores.begin(), waitSemaphores.end());
		VulkanContext::GetInstance().SubmitSwapchainPresent([this, s = std::move(semaphores)]()
			{
				// TODO: Avoid doing a call to vkQueuePresentKHR, instead generate
				//       a VkPresentInfoKHR and send it to VulkanContext wich should
				//       do a single call to vkQueuePresent

				VkPresentInfoKHR presentInfo{};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				presentInfo.waitSemaphoreCount = (uint32_t)s.size();
				presentInfo.pWaitSemaphores = s.data();
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = &m_Swapchain;
				presentInfo.pImageIndices = &m_CurrentFrameInFlight;

				VkQueue presentQueue = VulkanContext::GetInstance().GetPresentQueue();

				VkResult presentResult = vkQueuePresentKHR(presentQueue, &presentInfo);
				if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
				{
					Recreate();
				}
				else if (presentResult != VK_SUCCESS)
				{
					FLARE_CORE_ERROR("Failed to present");
				}
			});
	}

	void VulkanSwapchain::EnsureCreated()
	{
		FLARE_PROFILE_FUNCTION();

		if (m_Swapchain == VK_NULL_HANDLE)
		{
			Create();
		}
	}

	void VulkanSwapchain::Recreate()
	{
		FLARE_PROFILE_FUNCTION();

		VulkanContext::GetInstance().WaitForDevice();

		ReleaseImageViews();
		ReleaseSemaphores();
		Create();
	}

	void VulkanSwapchain::Create()
	{
		FLARE_PROFILE_FUNCTION();

		VulkanContext& context = VulkanContext::GetInstance();
		VkPhysicalDevice physicalDevice = context.GetPhysicalDevice();
		VkDevice device = context.GetDevice();

		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, m_Surface, &surfaceCapabilities));

		VkExtent2D swapChainExtent = GetExtent(surfaceCapabilities, m_WindowSize);
		FLARE_CORE_ASSERT(swapChainExtent.width > 0 && swapChainExtent.height > 0);

		m_Size.x = swapChainExtent.width;
		m_Size.y = swapChainExtent.height;

		uint32_t imageCount = glm::min(surfaceCapabilities.maxImageCount, surfaceCapabilities.minImageCount + 1);

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = m_ImageFormat;
		createInfo.imageColorSpace = m_ColorSpace;
		createInfo.imageExtent = swapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

		uint32_t graphicsQueueFamilyIndex = context.GetGraphicsQueueFamilyIndex();
		uint32_t presentQueueFamilyIndex = context.GetPresentQueueFamilyIndex();
		uint32_t queueFamilies[] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };

		if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
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
		createInfo.presentMode = m_PresentMode;
		createInfo.clipped = true;
		createInfo.oldSwapchain = m_Swapchain;

		VkSwapchainKHR oldSwapChain = m_Swapchain;

		VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &createInfo, nullptr, &m_Swapchain));

		uint32_t swapChainImageCount = 0;
		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, m_Swapchain, &swapChainImageCount, nullptr));

		m_FrameData.resize(swapChainImageCount);

		std::vector<VkImage> images;
		images.resize(swapChainImageCount);

		VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, m_Swapchain, &swapChainImageCount, images.data()));

		for (size_t i = 0; i < images.size(); i++)
		{
			m_FrameData[i].Image = images[i];
			context.SetDebugName(VK_OBJECT_TYPE_IMAGE, (uint64_t)m_FrameData[i].Image, fmt::format("SwapChainImage.{}", i).c_str());
		}

		if (oldSwapChain != VK_NULL_HANDLE)
		{
			vkDestroySwapchainKHR(device, oldSwapChain, nullptr);
		}

		CreateImageViews();
		CreateFrameBuffers();
		CreateSemaphores();
	}

	void VulkanSwapchain::CreateImageViews()
	{
		FLARE_PROFILE_FUNCTION();

		VulkanContext& context = VulkanContext::GetInstance();
		VkDevice device = context.GetDevice();

		for (size_t i = 0; i < m_FrameData.size(); i++)
		{
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.format = m_ImageFormat;
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.image = m_FrameData[i].Image;

			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_FrameData[i].ImageView));

			context.SetDebugName(
				VK_OBJECT_TYPE_IMAGE_VIEW,
				(uint64_t)m_FrameData[i].ImageView,
				fmt::format("SwapChainImageView.{}", i).c_str());
		}
	}

	void VulkanSwapchain::CreateFrameBuffers()
	{
		FLARE_PROFILE_FUNCTION();
		for (size_t i = 0; i < m_FrameData.size(); i++)
		{
			TextureSpecifications specifications{};
			specifications.Width = m_Size.x;
			specifications.Height = m_Size.y;
			specifications.Filtering = TextureFiltering::Closest;
			specifications.Wrap = TextureWrap::Clamp;
			specifications.Usage = TextureUsage::RenderTarget;
			specifications.GenerateMipMaps = false;
			specifications.Format = TextureFormat::RGBA8;

			Ref<Texture> attachmentTexture = CreateRef<VulkanTexture>(specifications, m_FrameData[i].Image, m_FrameData[i].ImageView);
			m_FrameData[i].FrameBuffer = CreateRef<VulkanFrameBuffer>(
				m_Size.x,
				m_Size.y,
				VulkanContext::GetInstance().GetColorOnlyPass(),
				Span<Ref<Texture>>(&attachmentTexture, 1),
				false);

			m_FrameData[i].FrameBuffer->SetDebugName(fmt::format("SwapChainFrameBuffer.{}", i));
		}
	}

	void VulkanSwapchain::CreateSemaphores()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(GetFrameCount() > 0);

		m_ImageAvailableSemaphores.resize(GetFrameCount());

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (size_t i = 0; i < m_ImageAvailableSemaphores.size(); i++)
		{
			vkCreateSemaphore(device, &createInfo, nullptr, &m_ImageAvailableSemaphores[i]);
		}
	}

	void VulkanSwapchain::Release()
	{
		FLARE_PROFILE_FUNCTION();
		if (m_Swapchain == VK_NULL_HANDLE)
			return;

		ReleaseImageViews();
		ReleaseSemaphores();

		vkDestroySwapchainKHR(VulkanContext::GetInstance().GetDevice(), m_Swapchain, nullptr);
		m_Swapchain = VK_NULL_HANDLE;
	}

	void VulkanSwapchain::ReleaseImageViews()
	{
		FLARE_PROFILE_FUNCTION();
		VulkanContext& context = VulkanContext::GetInstance();
		VkDevice device = context.GetDevice();

		for (size_t i = 0; i < m_FrameData.size(); i++)
		{
			vkDestroyImageView(device, m_FrameData[i].ImageView, nullptr);
			m_FrameData[i].ImageView = VK_NULL_HANDLE;
		}

		m_FrameData.clear();
	}

	void VulkanSwapchain::ReleaseSemaphores()
	{
		FLARE_PROFILE_FUNCTION();

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		for (VkSemaphore semaphore : m_ImageAvailableSemaphores)
			vkDestroySemaphore(device, semaphore, nullptr);

		m_ImageAvailableSemaphores.clear();
	}

	std::vector<VkSurfaceFormatKHR> VulkanSwapchain::GetSurfaceFormats()
	{
		FLARE_PROFILE_FUNCTION();
		VkPhysicalDevice device = VulkanContext::GetInstance().GetPhysicalDevice();

		uint32_t formatsCount;
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatsCount, nullptr));

		std::vector<VkSurfaceFormatKHR> formats(formatsCount);
		VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatsCount, formats.data()));

		return formats;
	}

	VkExtent2D VulkanSwapchain::GetExtent(const VkSurfaceCapabilitiesKHR& capabilities, glm::uvec2 size)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;

		VkExtent2D extent = { size.x, size.y };

		extent.width = glm::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = glm::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return extent;
	}

	uint32_t VulkanSwapchain::ChooseImageFormat(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		FLARE_PROFILE_FUNCTION();
		for (uint32_t i = 0; i < (uint32_t)formats.size(); i++)
		{
			if (formats[i].format == VK_FORMAT_R8G8B8A8_SRGB && formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
				return i;
		}

		return 0;
	}
}
