#include "ImGuiLayerVulkan.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Core/Application.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

#include <ImGuizmo.h>

#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_glfw.h>

namespace Flare
{
	void ImGuiLayerVulkan::InitializeRenderer()
	{
        FLARE_PROFILE_FUNCTION();

		{
			VkDescriptorPoolSize sizes[1];
			sizes[0].descriptorCount = 1;
			sizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

			VkDescriptorPoolCreateInfo info{};
			info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			info.maxSets = 20;
			info.poolSizeCount = 1;
			info.pPoolSizes = sizes;
			info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			VK_CHECK_RESULT(vkCreateDescriptorPool(VulkanContext::GetInstance().GetDevice(), &info, nullptr, &m_DescriptorPool));
		}

		Application& application = Application::GetInstance();
		Ref<Window> window = application.GetWindow();

		ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window->GetNativeWindow(), true);

		ImGui_ImplVulkan_InitInfo info{};
		info.CheckVkResultFn = [](VkResult vulkanResult) { VK_CHECK_RESULT(vulkanResult); };
		info.DescriptorPool = m_DescriptorPool;
		info.Instance = VulkanContext::GetInstance().GetVulkanInstance();
		info.PhysicalDevice = VulkanContext::GetInstance().GetPhysicalDevice();
		info.Queue = VulkanContext::GetInstance().GetGraphicsQueue();
		info.QueueFamily = VulkanContext::GetInstance().GetGraphicsQueueFamilyIndex();
		info.Device = VulkanContext::GetInstance().GetDevice();
		info.ImageCount = 2;
		info.MinImageCount = 2;
		ImGui_ImplVulkan_Init(&info, VulkanContext::GetInstance().GetColorOnlyPass()->GetHandle());

		VulkanContext::GetInstance().SetImageViewDeletionHandler([this](VkImageView imageView)
		{
			auto it = m_ImageToDescriptor.find((uint64_t)imageView);
			if (it != m_ImageToDescriptor.end())
			{
				ImGui_ImplVulkan_RemoveTexture(it->second);
				m_ImageToDescriptor.erase(it);
			}
		});
	}

	void ImGuiLayerVulkan::ShutdownRenderer()
	{
        FLARE_PROFILE_FUNCTION();
		VulkanContext::GetInstance().SetImageViewDeletionHandler(nullptr);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		vkDestroyDescriptorPool(VulkanContext::GetInstance().GetDevice(), m_DescriptorPool, nullptr);
	}

	void ImGuiLayerVulkan::InitializeFonts()
	{
        FLARE_PROFILE_FUNCTION();
		Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();
		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer->GetHandle());
		VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);
	}

	void ImGuiLayerVulkan::Begin()
	{
        FLARE_PROFILE_FUNCTION();
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	void ImGuiLayerVulkan::End()
	{
        FLARE_PROFILE_FUNCTION();
		ImGui::EndFrame();
		ImGui::Render();
	}

	void ImGuiLayerVulkan::RenderCurrentWindow()
	{
		FLARE_PROFILE_FUNCTION();
		ImDrawData* drawData = ImGui::GetDrawData();
		if (drawData == nullptr)
			return;

		VulkanContext& context = VulkanContext::GetInstance();

		Ref<VulkanCommandBuffer> commandBuffer = context.GetPrimaryCommandBuffer();
		commandBuffer->BeginRenderPass(context.GetColorOnlyPass(), context.GetSwapChainFrameBuffer(context.GetCurrentFrameInFlight()));

		ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer->GetHandle());

		commandBuffer->EndRenderPass();
	}

	void ImGuiLayerVulkan::UpdateWindows()
	{
		FLARE_PROFILE_FUNCTION();

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}

	ImTextureID ImGuiLayerVulkan::GetTextureId(const Ref<const Texture>& texture)
	{
		FLARE_CORE_ASSERT(texture);

		Ref<const VulkanTexture> vulkanTexture = As<const VulkanTexture>(texture);
		return GetImageId(vulkanTexture->GetImageViewHandle(), vulkanTexture->GetDefaultSampler());
	}

	ImTextureID ImGuiLayerVulkan::GetFrameBufferAttachmentId(const Ref<const FrameBuffer>& frameBuffer, uint32_t attachment)
	{
		FLARE_CORE_ASSERT(frameBuffer);
		FLARE_CORE_ASSERT(attachment < frameBuffer->GetAttachmentsCount());

		Ref<const VulkanFrameBuffer> vulkanFrameBuffer = As<const VulkanFrameBuffer>(frameBuffer);
		return GetImageId(vulkanFrameBuffer->GetAttachmentImageView(attachment), vulkanFrameBuffer->GetDefaultAttachmentSampler(attachment));
	}

	ImTextureID ImGuiLayerVulkan::GetImageId(VkImageView imageView, VkSampler defaultSampler)
	{
		auto it = m_ImageToDescriptor.find((uint64_t)imageView);
		if (it == m_ImageToDescriptor.end())
		{
			VkDescriptorSet descriptorSet = ImGui_ImplVulkan_AddTexture(defaultSampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			m_ImageToDescriptor.emplace((uint64_t)imageView, descriptorSet);

			FLARE_CORE_ASSERT(descriptorSet);

			return (ImTextureID)descriptorSet;
		}

		return (ImTextureID)it->second;
	}
}
