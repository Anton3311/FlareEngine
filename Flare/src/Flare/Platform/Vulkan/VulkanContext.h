#pragma once

#include "FlareCore/Assert.h"
#include "Flare/Renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define VK_CHECK_RESULT(expression) FLARE_CORE_ASSERT((expression) == VK_SUCCESS)

namespace Flare
{
	class FLARE_API VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(const GLFWwindow* window);
		~VulkanContext();

		void Initialize() override;
		void SwapBuffers() override;
	private:
		void CreateInstance();
		void CreateDebugMessenger();
	private:
		std::vector<VkLayerProperties> EnumerateAvailableLayers();
	private:
		bool m_DebugEnabled = true;

		PFN_vkCreateDebugUtilsMessengerEXT m_CreateDebugMessenger = nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT m_DestroyDebugMessenger = nullptr;

		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		const GLFWwindow* m_Window = nullptr;
		VkInstance m_Instance = VK_NULL_HANDLE;
	};
}
