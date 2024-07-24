#include "ImGuiVulkanRenderer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Flare/Platform/Vulkan/VulkanIndexBuffer.h"

#include "FlareEditor/ImGui/ImGuiLayerVulkan.h"

#include <backends/imgui_impl_vulkan.h>

namespace Flare
{
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

		m_Swapchain.Release();

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		vkDestroySurfaceKHR(VulkanContext::GetInstance().GetVulkanInstance(), m_Surface, nullptr);
		vkDestroyCommandPool(device, m_CommandPool, nullptr);

		for (const FrameData& frame : m_FrameData)
		{
			vkDestroySemaphore(device, frame.RenderCompleteSemaphore, nullptr);
		}

		m_FrameData.clear();
	}

	void ImGuiVulkanRenderer::Create(ImGuiViewport* viewport)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(m_FrameData.size() == 0);

		m_Swapchain.SetWindowSize(glm::uvec2((uint32_t)viewport->Size.x, (uint32_t)viewport->Size.y));
		m_Swapchain.EnsureCreated();

		m_FrameData.resize(m_Swapchain.GetFrameCount());

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < m_FrameData.size(); i++)
		{
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
		m_Swapchain.SubmitPresent(Span(&frameData.RenderCompleteSemaphore, 1), m_Swapchain.GetWindowSize(), true);
	}

	void ImGuiVulkanRenderer::RenderWindow(ImGuiViewport* viewport, void* renderArgs)
	{
		FLARE_PROFILE_FUNCTION();
		m_Swapchain.AcquireNextImage();

		VkDevice device = VulkanContext::GetInstance().GetDevice();

		// NOTE: Don't need to wait for a fence because everything is submitted in a single vkQueueSubmit,
		//       which is singals a fence in VulkanContext. VulkanContext waits for this fence at the start of the frame

		VK_CHECK_RESULT(vkResetCommandPool(device, m_CommandPool, 0));

		m_FrameDataIndex = (m_FrameDataIndex + 1) % m_Swapchain.GetFrameCount();

		FrameData& frameData = m_FrameData[m_FrameDataIndex];
		Ref<VulkanCommandBuffer> commandBuffer = frameData.CommandBuffer;

		commandBuffer->Begin();

		VkClearValue clearValue{};
		clearValue.color.float32[0] = 0.0f;
		clearValue.color.float32[1] = 0.0f;
		clearValue.color.float32[2] = 0.0f;
		clearValue.color.float32[3] = 1.0f;

		commandBuffer->BeginRenderPass(m_RenderPass, m_Swapchain.GetFrameBuffer(m_Swapchain.GetFrameInFlight()));

		RenderViewportData(viewport->DrawData, commandBuffer->GetHandle(), frameData.FrameResources);

		commandBuffer->EndRenderPass();
		commandBuffer->End();

		VkSemaphore waitSemaphores[] = { m_Swapchain.GetImageAvailableSemaphore() };

		VulkanContext::GetInstance().SubmitToGraphicsQueue(
			frameData.CommandBuffer,
			Span<const VkSemaphore>(waitSemaphores, 1),
			Span<const VkSemaphore>(&frameData.RenderCompleteSemaphore, 1),
			true);
	}

	// Based on ImGui_ImplVulkan_RenderDrawData
	void ImGuiVulkanRenderer::RenderViewportData(ImDrawData* drawData, VkCommandBuffer commandBuffer, Resources& frameResources)
	{
		FLARE_PROFILE_FUNCTION();

		glm::ivec2 viewportSize = glm::ivec2(
			(int)(drawData->DisplaySize.x * drawData->FramebufferScale.x),
			(int)(drawData->DisplaySize.y * drawData->FramebufferScale.y));

		if (viewportSize.x <= 0 || viewportSize.y <= 0)
			return;

		SetupFrameResources(drawData, frameResources);

		Ref<ImGuiLayerVulkan> imGuiLayer = As<ImGuiLayerVulkan>(ImGuiLayer::GetInstance());
		VkPipeline pipeline = imGuiLayer->GetPipeline();
		VkPipelineLayout pipelineLayout = imGuiLayer->GetPipelineLayout();
		VkDescriptorSet fontsDescriptor = imGuiLayer->GetFontsTextureDescrptor();

		SetupRenderState(drawData, frameResources, commandBuffer, pipeline, pipelineLayout, viewportSize);

		// Will project scissor/clipping rectangles into framebuffer space
		ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
		ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		// Render command lists
		// (Because we merged all buffers into a single one, we maintain our own offset into them)
		int global_vtx_offset = 0;
		int global_idx_offset = 0;
		for (int n = 0; n < drawData->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = drawData->CmdLists[n];
			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback != nullptr)
				{
					// User callback, registered via ImDrawList::AddCallback()
					// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
					if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
						SetupRenderState(drawData, frameResources, commandBuffer, pipeline, pipelineLayout, viewportSize);
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
					if (clip_max.x > viewportSize.x) { clip_max.x = (float)viewportSize.x; }
					if (clip_max.y > viewportSize.y) { clip_max.y = (float)viewportSize.y; }
					if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
						continue;

					// Apply scissor/clipping rectangle
					VkRect2D scissor;
					scissor.offset.x = (int32_t)(clip_min.x);
					scissor.offset.y = (int32_t)(clip_min.y);
					scissor.extent.width = (uint32_t)(clip_max.x - clip_min.x);
					scissor.extent.height = (uint32_t)(clip_max.y - clip_min.y);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

					// Bind DescriptorSet with font or user texture
					VkDescriptorSet desc_set[1] = { (VkDescriptorSet)pcmd->TextureId };
					if (sizeof(ImTextureID) < sizeof(ImU64))
					{
						// We don't support texture switches if ImTextureID hasn't been redefined to be 64-bit. Do a flaky check that other textures haven't been used.
						//IM_ASSERT(pcmd->TextureId == (ImTextureID)bd->FontDescriptorSet);

						FLARE_CORE_ASSERT(pcmd->TextureId == (ImTextureID)fontsDescriptor);

						desc_set[0] = fontsDescriptor;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, desc_set, 0, nullptr);

					// Draw
					vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
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
		VkRect2D scissor = { { 0, 0 }, { (uint32_t)viewportSize.x, (uint32_t)viewportSize.y} };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void ImGuiVulkanRenderer::SetupFrameResources(ImDrawData* drawData, Resources& resources)
	{
		FLARE_PROFILE_FUNCTION();

		if (drawData->TotalVtxCount > 0)
		{
			FLARE_PROFILE_SCOPE("SetupBuffers");

			size_t vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
			size_t indexBufferSize = drawData->TotalIdxCount * sizeof(ImDrawIdx);

			if (resources.VertexBuffer == nullptr)
			{
				resources.VertexBuffer = CreateRef<VulkanVertexBuffer>(vertexBufferSize, GPUBufferUsage::Dynamic);
				resources.VertexBuffer->GetBuffer().EnsureAllocated();
			}

			if (resources.IndexBuffer == nullptr)
			{
				if (sizeof(ImDrawIdx) == 2)
					resources.IndexBuffer = CreateRef<VulkanIndexBuffer>(IndexBuffer::IndexFormat::UInt32, indexBufferSize / 2, GPUBufferUsage::Dynamic);
				else
					resources.IndexBuffer = CreateRef<VulkanIndexBuffer>(IndexBuffer::IndexFormat::UInt16, indexBufferSize, GPUBufferUsage::Dynamic);

				resources.IndexBuffer->GetBuffer().EnsureAllocated();
			}

			if (resources.VertexBuffer->GetBuffer().GetSize() < vertexBufferSize)
				resources.VertexBuffer->GetBuffer().Resize(vertexBufferSize);

			if (resources.IndexBuffer->GetBuffer().GetSize() < indexBufferSize)
				resources.IndexBuffer->GetBuffer().Resize(indexBufferSize);

			size_t vertexOffset = 0;
			size_t indexOffset = 0;
			for (int32_t i = 0; i < drawData->CmdListsCount; i++)
			{
				const ImDrawList* cmd_list = drawData->CmdLists[i];

				size_t vertexDataSize = cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
				size_t indexDataSize = cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);

				resources.VertexBuffer->SetData(cmd_list->VtxBuffer.Data, vertexDataSize, vertexOffset);
				resources.IndexBuffer->SetData(MemorySpan::FromRawBytes(cmd_list->IdxBuffer.Data, indexDataSize), indexOffset);
				
				vertexOffset += vertexDataSize;
				indexOffset += indexDataSize;
			}
		}

	}

	void ImGuiVulkanRenderer::SetupRenderState(ImDrawData* drawData,
		Resources& frameResources,
		VkCommandBuffer commandBuffer,
		VkPipeline pipeline,
		VkPipelineLayout pipelineLayout,
		glm::ivec2 viewportSize)
	{
		FLARE_PROFILE_FUNCTION();
		// Bind pipeline:
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		}

		// Bind Vertex And Index Buffer:
		if (drawData->TotalVtxCount > 0)
		{
			VkBuffer vertex_buffers[1] = { frameResources.VertexBuffer->GetHandle() };
			VkDeviceSize vertex_offset[1] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertex_buffers, vertex_offset);
			vkCmdBindIndexBuffer(commandBuffer, frameResources.IndexBuffer->GetHandle(), 0, sizeof(ImDrawIdx) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		}

		// Setup viewport:
		{
			VkViewport viewport;
			viewport.x = 0;
			viewport.y = 0;
			viewport.width = (float)viewportSize.x;
			viewport.height = (float)viewportSize.y;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		}

		// Setup scale and translation:
		// Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
		{
			float scale[2];
			scale[0] = 2.0f / drawData->DisplaySize.x;
			scale[1] = 2.0f / drawData->DisplaySize.y;
			float translate[2];
			translate[0] = -1.0f - drawData->DisplayPos.x * scale[0];
			translate[1] = -1.0f - drawData->DisplayPos.y * scale[1];
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
		}
	}
}
