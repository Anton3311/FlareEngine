#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"
#include "FlareCore/Collections/Span.h"

#include <vulkan/vulkan.h>

#include <vector>
#include <glm/glm.hpp>

namespace Flare
{
	class VulkanFrameBuffer;
	class FLARE_API VulkanSwapchain
	{
	public:
		VulkanSwapchain(VkSurfaceKHR surface);
		~VulkanSwapchain();

		void Initialize();

		void Release();

		void SetWindowSize(glm::uvec2 windowSize) { m_WindowSize = windowSize; }
		void AcquireNextImage();

		void Present(Span<const VkSemaphore> waitSemaphores, glm::uvec2 windowSize);
		void SubmitPresent(Span<const VkSemaphore> waitSemaphores, glm::uvec2 windowSize, bool waitForMainRenderingSubmition);
		void HandlePresentResult(VkResult result);

		void EnsureCreated();
		void Recreate();

		inline void SetPresentMode(VkPresentModeKHR presentMode) { m_PresentMode = presentMode; }
		inline VkFormat GetImageFormat() const { return m_ImageFormat; }

		inline VkSwapchainKHR GetHandle() const { return m_Swapchain; }
		inline glm::uvec2 GetSize() const { return m_Size; }
		inline glm::uvec2 GetWindowSize() const { return m_WindowSize; }

		Ref<VulkanFrameBuffer> GetFrameBuffer(uint32_t index) const
		{
			FLARE_CORE_ASSERT(index < GetFrameCount());
			return m_FrameData[index].FrameBuffer;
		}

		VkImage GetImage(uint32_t index) const
		{
			FLARE_CORE_ASSERT(index < GetFrameCount());
			return m_FrameData[index].Image;
		}

		VkImageView GetImageView(uint32_t index) const
		{
			FLARE_CORE_ASSERT(index < GetFrameCount());
			return m_FrameData[index].ImageView;
		}

		uint32_t GetFrameCount() const { return (uint32_t)m_FrameData.size(); }
		uint32_t GetFrameInFlight() const { return m_CurrentFrameInFlight; }
		VkSemaphore GetImageAvailableSemaphore() const { return m_ImageAvailableSemaphores[m_CurrentImageAvailableSemaphore]; }
	private:
		void Create();
		void CreateImageViews();
		void CreateFrameBuffers();
		void CreateSemaphores();

		void ReleaseImageViews();
		void ReleaseSemaphores();

		std::vector<VkSurfaceFormatKHR> GetSurfaceFormats();

		VkExtent2D GetExtent(const VkSurfaceCapabilitiesKHR& capabilities, glm::uvec2 size);
		uint32_t ChooseImageFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	private:
		struct FrameData
		{
			VkImage Image = VK_NULL_HANDLE;
			VkImageView ImageView = VK_NULL_HANDLE;
			Ref<VulkanFrameBuffer> FrameBuffer = nullptr;
		};

		std::vector<FrameData> m_FrameData;

		uint32_t m_CurrentFrameInFlight = 0;

		uint32_t m_CurrentImageAvailableSemaphore = 0;
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;

		glm::uvec2 m_WindowSize = glm::uvec2(0, 0);
		glm::uvec2 m_Size = glm::uvec2(0, 0);

		VkPresentModeKHR m_PresentMode = VK_PRESENT_MODE_FIFO_KHR;
		VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR m_ColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
	};
}
