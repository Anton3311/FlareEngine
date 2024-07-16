#include "ImGuiVulkanRenderer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include <backends/imgui_impl_vulkan.h>

namespace Flare
{
	//
	// Modified ImGui Vulkan Implementation functions.
	// From imgui_impl_vulkan.cpp
	//

	void ImGui_ImplVulkan_RenderDrawData(ImDrawData* draw_data, VkCommandBuffer command_buffer, VkPipeline pipeline)
	{
		// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
		int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
		int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
		if (fb_width <= 0 || fb_height <= 0)
			return;

		ImGui_ImplVulkan_Data* bd = ImGui_ImplVulkan_GetBackendData();
		ImGui_ImplVulkan_InitInfo* v = &bd->VulkanInitInfo;
		if (pipeline == VK_NULL_HANDLE)
			pipeline = bd->Pipeline;

		// Allocate array to store enough vertex/index buffers. Each unique viewport gets its own storage.
		ImGui_ImplVulkan_ViewportData* viewport_renderer_data = (ImGui_ImplVulkan_ViewportData*)draw_data->OwnerViewport->RendererUserData;
		IM_ASSERT(viewport_renderer_data != nullptr);
		ImGui_ImplVulkanH_WindowRenderBuffers* wrb = &viewport_renderer_data->RenderBuffers;
		if (wrb->FrameRenderBuffers == nullptr)
		{
			wrb->Index = 0;
			wrb->Count = v->ImageCount;
			wrb->FrameRenderBuffers = (ImGui_ImplVulkanH_FrameRenderBuffers*)IM_ALLOC(sizeof(ImGui_ImplVulkanH_FrameRenderBuffers) * wrb->Count);
			memset(wrb->FrameRenderBuffers, 0, sizeof(ImGui_ImplVulkanH_FrameRenderBuffers) * wrb->Count);
		}
		IM_ASSERT(wrb->Count == v->ImageCount);
		wrb->Index = (wrb->Index + 1) % wrb->Count;
		ImGui_ImplVulkanH_FrameRenderBuffers* rb = &wrb->FrameRenderBuffers[wrb->Index];

		if (draw_data->TotalVtxCount > 0)
		{
			// Create or resize the vertex/index buffers
			size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
			size_t index_size = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
			if (rb->VertexBuffer == VK_NULL_HANDLE || rb->VertexBufferSize < vertex_size)
				CreateOrResizeBuffer(rb->VertexBuffer, rb->VertexBufferMemory, rb->VertexBufferSize, vertex_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
			if (rb->IndexBuffer == VK_NULL_HANDLE || rb->IndexBufferSize < index_size)
				CreateOrResizeBuffer(rb->IndexBuffer, rb->IndexBufferMemory, rb->IndexBufferSize, index_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

			// Upload vertex/index data into a single contiguous GPU buffer
			ImDrawVert* vtx_dst = nullptr;
			ImDrawIdx* idx_dst = nullptr;
			VkResult err = vkMapMemory(v->Device, rb->VertexBufferMemory, 0, rb->VertexBufferSize, 0, (void**)(&vtx_dst));
			check_vk_result(err);
			err = vkMapMemory(v->Device, rb->IndexBufferMemory, 0, rb->IndexBufferSize, 0, (void**)(&idx_dst));
			check_vk_result(err);
			for (int n = 0; n < draw_data->CmdListsCount; n++)
			{
				const ImDrawList* cmd_list = draw_data->CmdLists[n];
				memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
				vtx_dst += cmd_list->VtxBuffer.Size;
				idx_dst += cmd_list->IdxBuffer.Size;
			}
			VkMappedMemoryRange range[2] = {};
			range[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[0].memory = rb->VertexBufferMemory;
			range[0].size = VK_WHOLE_SIZE;
			range[1].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			range[1].memory = rb->IndexBufferMemory;
			range[1].size = VK_WHOLE_SIZE;
			err = vkFlushMappedMemoryRanges(v->Device, 2, range);
			check_vk_result(err);
			vkUnmapMemory(v->Device, rb->VertexBufferMemory);
			vkUnmapMemory(v->Device, rb->IndexBufferMemory);
		}

		// Setup desired Vulkan state
		ImGui_ImplVulkan_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);

		// Will project scissor/clipping rectangles into framebuffer space
		ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
		ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		// Render command lists
		// (Because we merged all buffers into a single one, we maintain our own offset into them)
		int global_vtx_offset = 0;
		int global_idx_offset = 0;
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback != nullptr)
				{
					// User callback, registered via ImDrawList::AddCallback()
					// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
					if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
						ImGui_ImplVulkan_SetupRenderState(draw_data, pipeline, command_buffer, rb, fb_width, fb_height);
					else
						pcmd->UserCallback(cmd_list, pcmd);
				}
				else
				{
					// Project scissor/clipping rectangles into framebuffer space
					ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
					ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

					// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
					if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
					if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
					if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
					if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
					if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
						continue;

					// Apply scissor/clipping rectangle
					VkRect2D scissor;
					scissor.offset.x = (int32_t)(clip_min.x);
					scissor.offset.y = (int32_t)(clip_min.y);
					scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
					scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
					vkCmdSetScissor(command_buffer, 0, 1, &scissor);

					// Bind DescriptorSet with font or user texture
					VkDescriptorSet desc_set[1] = { (VkDescriptorSet)pcmd->TextureId };
					if (sizeof(ImTextureID) < sizeof(ImU64))
					{
						// We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures haven't been used.
						IM_ASSERT(pcmd->TextureId == (ImTextureID)bd->FontDescriptorSet);
						desc_set[0] = bd->FontDescriptorSet;
					}
					vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->PipelineLayout, 0, 1, desc_set, 0, nullptr);

					// Draw
					vkCmdDrawIndexed(command_buffer, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
				}
			}
			global_idx_offset += cmd_list->IdxBuffer.Size;
			global_vtx_offset += cmd_list->VtxBuffer.Size;
		}

		// Note: at this point both vkCmdSetViewport() and vkCmdSetScissor() have been called.
		// Our last values will leak into user/application rendering IF:
		// - Your app uses a pipeline with VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR dynamic state
		// - And you forgot to call vkCmdSetViewport() and vkCmdSetScissor() yourself to explicitly set that state.
		// If you use VK_DYNAMIC_STATE_VIEWPORT or VK_DYNAMIC_STATE_SCISSOR you are responsible for setting the values before rendering.
		// In theory we should aim to backup/restore those values but I am not sure this is possible.
		// We perform a call to vkCmdSetScissor() to set back a full viewport which is likely to fix things for 99% users but technically this is not perfect. (See github #4644)
		VkRect2D scissor = { { 0, 0 }, { (uint32_t)fb_width, (uint32_t)fb_height } };
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);
	}

	//
	// ImGuiVulkanRenderer
	//

	ImGuiVulkanRenderer::ImGuiVulkanRenderer(VkSurfaceKHR surface)
		: m_Surface(surface), m_Swapchain(surface)
	{
		FLARE_PROFILE_FUNCTION();
		m_Swapchain.SetPresentMode(VK_PRESENT_MODE_FIFO_KHR);
		m_Swapchain.Initialize();

		VkCommandPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.queueFamilyIndex = VulkanContext::GetInstance().GetGraphicsQueueFamilyIndex();

		VK_CHECK_RESULT(vkCreateCommandPool(
			VulkanContext::GetInstance().GetDevice(),
			&createInfo,
			nullptr,
			&m_CommandPool));

		m_RenderPass = VulkanContext::GetInstance().GetColorOnlyPass();
	}

	ImGuiVulkanRenderer::~ImGuiVulkanRenderer()
	{
		FLARE_PROFILE_FUNCTION();

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		vkDestroySurfaceKHR(VulkanContext::GetInstance().GetVulkanInstance(), m_Surface, nullptr);
		vkDestroyCommandPool(device, m_CommandPool, nullptr);

		for (const FrameData& frame : m_FrameData)
		{
			vkDestroySemaphore(device, frame.RenderCompleteSemaphore, nullptr);
			vkDestroyFence(device, frame.FrameFence, nullptr);
		}

		m_FrameData.clear();
	}

	void ImGuiVulkanRenderer::Create(ImGuiViewport* viewport)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(m_FrameData.size() == 0);

		m_Swapchain.SetWindowSize(glm::uvec2((uint32_t)viewport->Size.x, (uint32_t)viewport->Size.y));
		m_Swapchain.Recreate();

		m_FrameData.resize(m_Swapchain.GetFrameCount());

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < m_FrameData.size(); i++)
		{
			vkCreateFence(device, &fenceCreateInfo, nullptr, &m_FrameData[i].FrameFence);
			vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_FrameData[i].RenderCompleteSemaphore);

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			VkCommandBufferAllocateInfo allocationInfo{};
			allocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocationInfo.commandPool = m_CommandPool;
			allocationInfo.commandBufferCount = 1;
			allocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &allocationInfo, &commandBuffer));

			m_FrameData[i].CommandBuffer = CreateRef<VulkanCommandBuffer>(commandBuffer);
		}
	}

	void ImGuiVulkanRenderer::SetSize(glm::uvec2 size)
	{
		FLARE_PROFILE_FUNCTION();
		m_Swapchain.SetWindowSize(size);
	}

	void ImGuiVulkanRenderer::Present()
	{
		FLARE_PROFILE_FUNCTION();

		const FrameData& frameData = m_FrameData[m_FrameDataIndex];
		m_Swapchain.Present(Span(&frameData.RenderCompleteSemaphore, 1), m_Swapchain.GetWindowSize());
	}

	void ImGuiVulkanRenderer::RenderWindow(ImGuiViewport* viewport, void* renderArgs)
	{
		FLARE_PROFILE_FUNCTION();
		m_Swapchain.AcquireNextImage();

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		VK_CHECK_RESULT(vkWaitForFences(device, 1, &m_FrameData[m_FrameDataIndex].FrameFence, VK_TRUE, UINT64_MAX));
		VK_CHECK_RESULT(vkResetCommandPool(device, m_CommandPool, 0));

		m_FrameDataIndex = (m_FrameDataIndex + 1) % m_Swapchain.GetFrameCount();

		const FrameData& frameData = m_FrameData[m_FrameDataIndex];
		Ref<VulkanCommandBuffer> commandBuffer = frameData.CommandBuffer;


		commandBuffer->Begin();

		VkClearValue clearValue{};
		clearValue.color.float32[0] = 0.0f;
		clearValue.color.float32[1] = 0.0f;
		clearValue.color.float32[2] = 0.0f;
		clearValue.color.float32[3] = 1.0f;

		commandBuffer->BeginRenderPass(m_RenderPass, m_Swapchain.GetFrameBuffer(m_Swapchain.GetFrameInFlight()));

		ImGui_ImplVulkan_RenderDrawData(viewport->DrawData, frameData.CommandBuffer->GetHandle(), nullptr);

		commandBuffer->EndRenderPass();

		VK_CHECK_RESULT(vkResetFences(device, 1, &frameData.FrameFence));

		VkCommandBuffer commandBufferHandle = commandBuffer->GetHandle();
		VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSemaphore waitSemaphores[] =
		{
			m_Swapchain.GetImageAvailableSemaphore(),
			VulkanContext::GetInstance().GetRenderCompleteSemaphore()
		};

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBufferHandle;
		submitInfo.pWaitDstStageMask = &waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &frameData.RenderCompleteSemaphore;
		submitInfo.waitSemaphoreCount = 2;
		submitInfo.pWaitSemaphores = waitSemaphores;

		VK_CHECK_RESULT(vkQueueSubmit(
			VulkanContext::GetInstance().GetGraphicsQueue(),
			1, &submitInfo,
			frameData.FrameFence));
	}
}