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
		struct FrameData
		{
			VkSemaphore RenderCompleteSemaphore = VK_NULL_HANDLE;
			Ref<VulkanCommandBuffer> CommandBuffer = nullptr;
			Ref<VulkanVertexBuffer> VertexBuffer = nullptr;
			Ref<VulkanIndexBuffer> IndexBuffer = nullptr;
		};

		ImGuiVulkanRenderer(VkSurfaceKHR surface);
		~ImGuiVulkanRenderer();

		void Create(ImGuiViewport* viewport);
		void SetSize(glm::uvec2 size);
		void Present();
		void RenderWindow(ImGuiViewport* viewport, void* renderArgs);
	private:
		void RenderViewportData(ImDrawData* drawData, FrameData& frameData);
		void SetupRenderState(ImDrawData* drawData,
			const FrameData& frameData,
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
