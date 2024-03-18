#pragma once

#include "FlareCore/Assert.h"
#include "Flare/Renderer/GraphicsContext.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define VK_CHECK_RESULT(expression) FLARE_CORE_ASSERT((expression) == VK_SUCCESS)

namespace Flare
{
	class FLARE_API VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(GLFWwindow* window);
		~VulkanContext();

		void Initialize() override;
		void SwapBuffers() override;
	private:
		void CreateInstance();
		void CreateDebugMessenger();
		void CreateSurface();
	private:
		std::vector<VkLayerProperties> EnumerateAvailableLayers();
	private:
		bool m_DebugEnabled = true;

		PFN_vkCreateDebugUtilsMessengerEXT m_CreateDebugMessenger = nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT m_DestroyDebugMessenger = nullptr;

		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		GLFWwindow* m_Window = nullptr;
		VkInstance m_Instance = VK_NULL_HANDLE;
	};
}
