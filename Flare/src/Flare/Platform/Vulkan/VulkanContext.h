#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <optional>

#define VK_CHECK_RESULT(expression) FLARE_CORE_ASSERT((expression) == VK_SUCCESS)

namespace Flare
{
	class FLARE_API VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* window);
		~VulkanContext();

		void Initialize() override;
		void Release() override;
		void BeginFrame() override;
		void Present() override;

		void WaitForDevice();

		Ref<VulkanCommandBuffer> GetPrimaryCommandBuffer() const { return m_PrimaryCommandBuffer; }

		VkInstance GetVulkanInstance() const { return m_Instance; }
		VkDevice GetDevice() const { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }

		static VulkanContext& GetInstance() { return *(VulkanContext*)&GraphicsContext::GetInstance(); }
	private:
		void CreateInstance(const Span<const char*>& enabledLayers);
		void CreateDebugMessenger();
		void CreateSurface();
		void ChoosePhysicalDevice();
		void GetQueueFamilyProperties();
		void CreateLogicalDevice(const Span<const char*>& enabledLayers, const Span<const char*>& enabledExtensions);
	
		void CreateCommandBufferPool();
		VkCommandBuffer CreateCommandBuffer();
		void CreateSyncObjects();

		void CreateSwapChain();
		void RecreateSwapChain();
		void ReleaseSwapChain();
		void CreateSwapChainImageViews();

		uint32_t ChooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& formats);
		VkExtent2D GetSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkPresentModeKHR ChoosePrensentMode(const std::vector<VkPresentModeKHR>& modes);
	private:
		std::vector<VkLayerProperties> EnumerateAvailableLayers();
	private:
		bool m_DebugEnabled = true;
		uint32_t m_FramesInFlight = 0;
		uint32_t m_CurrentFrameInFlight = 0;

		PFN_vkCreateDebugUtilsMessengerEXT m_CreateDebugMessenger = nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT m_DestroyDebugMessenger = nullptr;

		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		GLFWwindow* m_Window = nullptr;
		VkInstance m_Instance = VK_NULL_HANDLE;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;

		std::optional<uint32_t> m_GraphicsQueueFamilyIndex;
		std::optional<uint32_t> m_PresentQueueFamilyIndex;

		// Swap chain
		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		std::vector<Ref<VulkanFrameBuffer>> m_SwapChainFrameBuffers;

		glm::uvec2 m_SwapChainExtent = glm::uvec2(0);
		VkFormat m_SwapChainImageFormat = VK_FORMAT_UNDEFINED;

		VkFence m_FrameFence = VK_NULL_HANDLE;
		VkSemaphore m_ImageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore m_RenderFinishedSemaphore = VK_NULL_HANDLE;

		// Command buffers
		VkCommandPool m_CommandBufferPool = VK_NULL_HANDLE;
		Ref<VulkanCommandBuffer> m_PrimaryCommandBuffer = nullptr;

		// Render pass
		Ref<VulkanRenderPass> m_ColorOnlyPass = nullptr;
	};
}
