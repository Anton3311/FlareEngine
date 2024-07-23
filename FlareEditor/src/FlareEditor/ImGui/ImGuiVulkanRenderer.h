#pragma once

#include "Flare/Platform/Vulkan/VulkanSwapchain.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <vector>

namespace Flare
{
	class VulkanVertexBuffer;
	class VulkanIndexBuffer;
	class VulkanRenderPass;
	class VulkanCommandBuffer;
	class ImGuiVulkanRenderer
	{
	public:
		struct Resources
		{
			Ref<VulkanVertexBuffer> VertexBuffer = nullptr;
			Ref<VulkanIndexBuffer> IndexBuffer = nullptr;
		};

		struct FrameData
		{
			VkSemaphore RenderCompleteSemaphore = VK_NULL_HANDLE;
			Ref<VulkanCommandBuffer> CommandBuffer = nullptr;
			Resources FrameResources;
		};

		ImGuiVulkanRenderer(VkSurfaceKHR surface);
		~ImGuiVulkanRenderer();

		void Create(ImGuiViewport* viewport);
		void SetSize(glm::uvec2 size);
		void Present();
		void RenderWindow(ImGuiViewport* viewport, void* renderArgs);

		static void RenderViewportData(ImDrawData* drawData, VkCommandBuffer commandBuffer, Resources& frameResources);
	private:
		static void SetupFrameResources(ImDrawData* drawData, Resources& resources);
		static void SetupRenderState(ImDrawData* drawData,
			Resources& frameResources,
			VkCommandBuffer commandBuffer,
			VkPipeline pipeline,
			VkPipelineLayout pipelineLayout,
			glm::ivec2 viewportSize);
	private:
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		VulkanSwapchain m_Swapchain;

		uint32_t m_FrameDataIndex = 0;

		Ref<VulkanRenderPass> m_RenderPass = nullptr;

		std::vector<FrameData> m_FrameData;
	};
}
