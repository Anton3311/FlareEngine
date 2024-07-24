#include "VulkanContext.h"

#include "FlareCore/Profiler/Profiler.h"
#include "Flare/Core/Application.h"

#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Flare
{
	static VkBool32 VulkanDebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
			return VK_FALSE;

		const char* messageType = "";
		switch (messageTypes)
		{
		case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
			messageType = "General";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
			messageType = "Performance";
			break;
		case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
			messageType = "Validation";
			break;
		}

		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			FLARE_CORE_INFO("Validation layers[Info]: {} {}", messageType, pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			FLARE_CORE_ERROR("Validation layers[Error]: {} {}", messageType, pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			FLARE_CORE_WARN("Validation layers[Warning]: {} {}", messageType, pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			FLARE_CORE_TRACE("Validation layers[Verbose]: {} {}", messageType, pCallbackData->pMessage);
			break;
		}

		return VK_FALSE;
	}



	VulkanContext::VulkanContext(Ref<Window> window)
		: m_Window(window),
		m_VSyncEnabled(window->GetProperties().VSyncEnabled),
		m_StagingBufferPool(1024 * 32, 4)
	{
	}

	VulkanContext::~VulkanContext()
	{
	}

	void VulkanContext::Initialize()
	{
		FLARE_PROFILE_FUNCTION();

		VkPhysicalDeviceType deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

		const auto& commandLineArguments = Application::GetInstance().GetCommandLineArguments();
		for (uint32_t i = 0; i < commandLineArguments.ArgumentsCount; i++)
		{
			const char* argument = commandLineArguments.Arguments[i];
			if (std::strcmp(argument, "--vulkan-debug") == 0)
			{
				m_DebugEnabled = true;
			}

			if (std::strcmp(commandLineArguments.Arguments[i], "--device=discrete") == 0)
			{
				deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
			}

			if (std::strcmp(commandLineArguments.Arguments[i], "--device=integrated") == 0)
			{
				deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
			}
		}

		std::vector<VkLayerProperties> supportedLayers = EnumerateAvailableLayers();
		std::vector<const char*> enabledLayers;
		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

		for (const auto& l : supportedLayers)
		{
			FLARE_CORE_INFO(l.layerName);
		}

		auto addIfSupported = [&](const char* layerName)
		{
			if (std::find_if(
				supportedLayers.begin(),
				supportedLayers.end(),
				[&](const VkLayerProperties& a) -> bool { return std::strcmp(a.layerName, layerName) == 0; }) != supportedLayers.end())
			{
				enabledLayers.push_back(layerName);
			}
		};

		if (m_DebugEnabled)
		{
			addIfSupported(validationLayerName);
		}

		CreateInstance(Span<const char*>::FromVector(enabledLayers));

		if (m_DebugEnabled)
		{
			CreateDebugMessenger();

			m_SetDebugNameFunction = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(m_Instance, "vkSetDebugUtilsObjectNameEXT"));
			FLARE_CORE_ASSERT(m_SetDebugNameFunction);
		}

		CreateSurface();
		ChoosePhysicalDevice(deviceType);
		GetQueueFamilyProperties();

		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);

			FLARE_CORE_INFO("Physical device name: {}", properties.deviceName);
		}

		std::vector<const char*> deviceExtensions =
		{
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_MAINTENANCE1_EXTENSION_NAME,

			// NOTE: These are used by Vulkan Memory Allocator
			//
			// Initially they weren't provided and the allocator was still working,
			// but was crashing when debugging with RenderDoc
			VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
			VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
		};

		CreateLogicalDevice(Span<const char*>::FromVector(enabledLayers), Span<const char*>::FromVector(deviceExtensions));

		CreateMemoryAllocator();

		m_Swapchain = CreateScope<VulkanSwapchain>(m_Surface);
		m_Swapchain->Initialize();

		{
			VkAttachmentDescription attachment{};
			attachment.flags = 0;
			attachment.format = m_Swapchain->GetImageFormat();
			attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			m_ColorOnlyPass = CreateRef<VulkanRenderPass>(Span<VkAttachmentDescription>(attachment));
			
			VkClearValue clearValue{};
			clearValue.color.float32[0] = 0.0f;
			clearValue.color.float32[1] = 0.0f;
			clearValue.color.float32[2] = 0.0f;
			clearValue.color.float32[3] = 0.0f;

			m_ColorOnlyPass->SetDebugName("ColorOnlyPass");
			m_ColorOnlyPass->SetDefaultClearValues(Span(&clearValue, 1));
		}

		m_Swapchain->EnsureCreated();

		CreateSyncObjects();

		m_CurrentSyncObjects = m_SyncObjects[m_CurrentFrameSyncObjectsIndex];

		CreateCommandBufferPool();

		m_PrimaryCommandBuffer = CreateRef<VulkanCommandBuffer>(CreateCommandBuffer());

		VkDescriptorSetLayoutBinding emptyBinding{};
		emptyBinding.binding = 0;
		emptyBinding.descriptorCount = 1;
		emptyBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		emptyBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

		m_EmptyDescriptorSetPool = CreateRef<VulkanDescriptorSetPool>(1, Span(&emptyBinding, 1));
		m_EmptyDescriptorSetLayout = CreateRef<VulkanDescriptorSetLayout>(Span<VkDescriptorSetLayoutBinding>());
		m_EmptyDescriptorSet = As<VulkanDescriptorSetPool>(m_EmptyDescriptorSetPool)->AllocateSet(m_EmptyDescriptorSetLayout);
	}

	void VulkanContext::Release()
	{
		WaitForDevice();

		m_EmptyDescriptorSetPool->ReleaseSet(m_EmptyDescriptorSet);
		m_EmptyDescriptorSet = nullptr;
		m_EmptyDescriptorSetLayout = nullptr;
		m_EmptyDescriptorSetPool = nullptr;

		m_RenderPassCache.Clear();
		m_StagingBufferPool.Release();

		m_DefaultPipelines.clear();

		if (m_DebugMessenger)
			m_DestroyDebugMessenger(m_Instance, m_DebugMessenger, nullptr);

		m_RenderPasses.clear();
		m_ColorOnlyPass = nullptr;

		VkCommandBuffer commandBuffer = m_PrimaryCommandBuffer->GetHandle();
		vkFreeCommandBuffers(m_Device, m_CommandBufferPool, 1, &commandBuffer);
		vkDestroyCommandPool(m_Device, m_CommandBufferPool, nullptr);
		m_PrimaryCommandBuffer = nullptr;

		vmaDestroyAllocator(m_Allocator);

		m_Swapchain.reset();

		for (const FrameSyncObjects& objects : m_SyncObjects)
		{
			vkDestroySemaphore(m_Device, objects.RenderingCompleteSemaphore, nullptr);
			vkDestroySemaphore(m_Device, objects.RenderingCompleteSemaphore2, nullptr);
			vkDestroyFence(m_Device, objects.FrameFence, nullptr);
		}

		m_SyncObjects.clear();
		m_CurrentSyncObjects = {};

		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

		vkDestroyDevice(m_Device, nullptr);
		vkDestroyInstance(m_Instance, nullptr);

		m_Device = VK_NULL_HANDLE;
	}

	void VulkanContext::BeginFrame()
	{
		FLARE_PROFILE_FUNCTION();

		if (!m_Window->GetProperties().IsMinimized)
		{
			{
				FLARE_PROFILE_SCOPE("WaitForFence");

				if (!m_SkipWaitForFrameFence)
				{
					// NOTE: Skip waiting for the fence in order to avoid a dead lock.
					//       This can happen, if in the previous frame the window was
					//       minimized and nothing was submitted for rendering, which
					//       means that the frame fence wasn't signalled, thus it leads
					//       to waiting for an unsugnalled fence in the current frame.
					VK_CHECK_RESULT(vkWaitForFences(m_Device, 1, &m_CurrentSyncObjects.FrameFence, VK_TRUE, UINT64_MAX));
				}

				m_SkipWaitForFrameFence = false;
			}

			m_CurrentFrameSyncObjectsIndex = (m_CurrentFrameSyncObjectsIndex + 1) % m_Swapchain->GetFrameCount();
			m_CurrentSyncObjects = m_SyncObjects[m_CurrentFrameSyncObjectsIndex];

			VK_CHECK_RESULT(vkResetFences(m_Device, 1, &m_CurrentSyncObjects.FrameFence));

			m_Swapchain->AcquireNextImage();
		}

		m_PrimaryCommandBuffer->Reset();

		m_StagingBufferPool.Reset();

		m_PrimaryCommandBuffer->Begin();
	}

	void VulkanContext::EndFrame()
	{
		FLARE_PROFILE_FUNCTION();

		m_PrimaryCommandBuffer->End();
		m_StagingBufferPool.FlushMemory();

		if (m_Window->GetProperties().IsMinimized)
		{
			m_SkipWaitForFrameFence = true;
			return;
		}

		{
			FLARE_PROFILE_SCOPE("Submit");

			VkCommandBuffer commandBufferHandle = m_PrimaryCommandBuffer->GetHandle();

			VkSemaphore waitSemaphore = m_Swapchain->GetImageAvailableSemaphore();
			VkSemaphore signalSemaphores[] = { m_CurrentSyncObjects.RenderingCompleteSemaphore, m_CurrentSyncObjects.RenderingCompleteSemaphore2 };

			VkPipelineStageFlags colorAttachmentStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			uint32_t maxWaitStagesCount = 0;
			for (const auto& submition : m_GraphicsQueueSubmitions)
			{
				maxWaitStagesCount = glm::max(maxWaitStagesCount, submition.WaitSemaphoreCount);
			}

			std::vector<VkPipelineStageFlags> waitStages;
			waitStages.assign(maxWaitStagesCount, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBufferHandle;
			submitInfo.pWaitDstStageMask = &colorAttachmentStage;
			submitInfo.signalSemaphoreCount = m_SignalSecondarySemaphore ? 2 : 1;
			submitInfo.pSignalSemaphores = signalSemaphores;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &waitSemaphore;

			std::vector<VkSubmitInfo> submitInfos;
			std::vector<VkCommandBuffer> commandBuffers;
			for (const auto& submition : m_GraphicsQueueSubmitions)
			{
				VkSubmitInfo& submitInfo = submitInfos.emplace_back();
				submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				submitInfo.commandBufferCount = 1;
				submitInfo.pCommandBuffers = VK_NULL_HANDLE;
				submitInfo.pWaitDstStageMask = waitStages.data();
				submitInfo.waitSemaphoreCount = submition.WaitSemaphoreCount;
				submitInfo.pWaitSemaphores = m_UsedSemaphores.data() + submition.FirstWaitSemaphore;
				submitInfo.signalSemaphoreCount = submition.SignalSemaphoreCount;
				submitInfo.pSignalSemaphores = m_UsedSemaphores.data() + submition.FirstSignalSemaphore;

				commandBuffers.push_back(As<const VulkanCommandBuffer>(submition.CommandBuffer)->GetHandle());
			}

			for (size_t i = 0; i < submitInfos.size(); i++)
			{
				submitInfos[i].pCommandBuffers = &commandBuffers[i];
			}

			submitInfos.insert(submitInfos.begin(), submitInfo);

			VK_CHECK_RESULT(vkQueueSubmit(m_GraphicsQueue, (uint32_t)submitInfos.size(), submitInfos.data(), m_CurrentSyncObjects.FrameFence));

			m_SignalSecondarySemaphore = false;
			m_GraphicsQueueSubmitions.clear();
		}

	}

	void VulkanContext::Present()
	{
		FLARE_PROFILE_FUNCTION();

		int32_t width, height;
		glfwGetFramebufferSize((GLFWwindow*)m_Window->GetNativeWindow(), &width, &height);

		m_Swapchain->SubmitPresent(Span<const VkSemaphore>(&m_CurrentSyncObjects.RenderingCompleteSemaphore, 1), glm::uvec2((uint32_t)width, (uint32_t)height));

		{
			FLARE_PROFILE_SCOPE("Present");
			for (size_t i = 0; i < m_PresentSubmitions.size(); i++)
			{
				PresentSubmition& submition = m_PresentSubmitions[i];

				VkPresentInfoKHR presentInfo{};
				presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
				presentInfo.swapchainCount = 1;
				presentInfo.pSwapchains = &submition.SwapchainHandle;
				presentInfo.pImageIndices = &submition.ImageIndex;
				presentInfo.waitSemaphoreCount = submition.WaitSemaphoreCount;
				presentInfo.pWaitSemaphores = m_UsedSemaphores.data() + submition.FirstWaitSemaphore;

				VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
				submition.Swapchain.HandlePresentResult(result);
			}

			m_PresentSubmitions.clear();
		}

		{
			FLARE_PROFILE_SCOPE("WaitIdle");
			VkResult waitResult = vkQueueWaitIdle(m_PresentQueue);
			if (waitResult != VK_SUCCESS)
			{
				FLARE_CORE_ERROR("Failed with result: {}", (std::underlying_type_t<VkResult>)waitResult);
				FLARE_CORE_ASSERT(false);
			}
		}

		glfwSwapBuffers((GLFWwindow*)m_Window->GetNativeWindow());

		if (m_VSyncEnabled != m_Window->GetProperties().VSyncEnabled)
		{
			VkPresentModeKHR presentMode = m_Window->GetProperties().VSyncEnabled
				? VK_PRESENT_MODE_FIFO_KHR
				: VK_PRESENT_MODE_MAILBOX_KHR;

			m_Swapchain->SetPresentMode(presentMode);
			m_Swapchain->Recreate();

			m_VSyncEnabled = m_Window->GetProperties().VSyncEnabled;
		}
	}

	void VulkanContext::WaitForDevice()
	{
		FLARE_PROFILE_FUNCTION();
		VK_CHECK_RESULT(vkDeviceWaitIdle(m_Device));
	}

	Ref<CommandBuffer> VulkanContext::GetCommandBuffer() const
	{
		return m_PrimaryCommandBuffer;
	}

	void VulkanContext::SubmitToGraphicsQueue(Ref<CommandBuffer> commandBuffer, Span<const VkSemaphore> waitSempahores, Span<const VkSemaphore> signalSemaphores)
	{
		GraphicsQueueSubmition& submition = m_GraphicsQueueSubmitions.emplace_back();
		submition.CommandBuffer = commandBuffer;
		submition.FirstSignalSemaphore = (uint32_t)m_UsedSemaphores.size();
		submition.SignalSemaphoreCount = (uint32_t)signalSemaphores.GetSize();

		for (VkSemaphore semaphore : signalSemaphores)
			m_UsedSemaphores.push_back(semaphore);

		submition.FirstWaitSemaphore = (uint32_t)m_UsedSemaphores.size();
		submition.WaitSemaphoreCount = (uint32_t)waitSempahores.GetSize();

		for (VkSemaphore semaphore : waitSempahores)
			m_UsedSemaphores.push_back(semaphore);
	}

	void VulkanContext::SubmitSwapchainPresent(VulkanSwapchain& swapchain, Span<const VkSemaphore> waitSemaphores)
	{
		FLARE_PROFILE_FUNCTION();

		m_PresentSubmitions.emplace_back(swapchain, (uint32_t)m_UsedSemaphores.size(), (uint32_t)waitSemaphores.GetSize());

		for (VkSemaphore semaphore : waitSemaphores)
			m_UsedSemaphores.push_back(semaphore);
	}

	Ref<VulkanCommandBuffer> VulkanContext::BeginTemporaryCommandBuffer()
	{
		FLARE_PROFILE_FUNCTION();
		VkCommandBuffer commandBuffer;

		VkCommandBufferAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		info.commandBufferCount = 1;
		info.commandPool = m_CommandBufferPool;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_Device, &info, &commandBuffer));

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &beginInfo));
		return CreateRef<VulkanCommandBuffer>(commandBuffer);
	}

	void VulkanContext::EndTemporaryCommandBuffer(Ref<VulkanCommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		commandBuffer->End();

		VkCommandBuffer buffer = commandBuffer->GetHandle();

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &buffer;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		{
			FLARE_PROFILE_SCOPE("QueueSubmit");
			VK_CHECK_RESULT(vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, nullptr));
		}

		{
			FLARE_PROFILE_SCOPE("QueueWaitIdle");
			VK_CHECK_RESULT(vkQueueWaitIdle(m_GraphicsQueue));
		}

		vkFreeCommandBuffers(m_Device, m_CommandBufferPool, 1, &buffer);
	}

	Ref<VulkanRenderPass> VulkanContext::FindOrCreateRenderPass(Span<TextureFormat> formats)
	{
		FLARE_PROFILE_FUNCTION();
		RenderPassKey key;
		key.Formats.assign(formats.begin(), formats.end());

		auto it = m_RenderPasses.find(key);
		if (it == m_RenderPasses.end())
		{
			std::vector<VkAttachmentDescription> descriptions(formats.GetSize());
			std::optional<uint32_t> depthAttachmentIndex = {};
			for (size_t i = 0; i < descriptions.size(); i++)
			{
				auto& description = descriptions[i];

				description.format = TextureFormatToVulkanFormat(formats[i]);
				description.samples = VK_SAMPLE_COUNT_1_BIT;
				description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

				if (IsDepthTextureFormat(formats[i]))
				{
					depthAttachmentIndex = (uint32_t)i;
					description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
					description.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else
				{
					description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
					description.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				}
			}

			Ref<VulkanRenderPass> renderPass = CreateRef<VulkanRenderPass>(Span<VkAttachmentDescription>::FromVector(descriptions), depthAttachmentIndex);
			m_RenderPasses.emplace(key, renderPass);

			return renderPass;
		}
		else
		{
			return it->second;
		}
	}

	void VulkanContext::NotifyImageViewDeletionHandler(VkImageView deletedImageView)
	{
		FLARE_PROFILE_FUNCTION();
		if (m_ImageDeletationHandler)
		{
			m_ImageDeletationHandler(deletedImageView);
		}
	}

	VkResult VulkanContext::SetDebugName(VkObjectType objectType, uint64_t objectHandle, const char* name)
	{
		FLARE_PROFILE_FUNCTION();
		if (m_DebugEnabled)
		{
			if (m_SetDebugNameFunction == nullptr)
			{
				return VK_ERROR_EXTENSION_NOT_PRESENT;
			}

			VkDebugUtilsObjectNameInfoEXT info{};
			info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
			info.pObjectName = name;
			info.objectType = objectType;
			info.objectHandle = objectHandle;

			return m_SetDebugNameFunction(m_Device, &info);
		}

		return VK_SUCCESS;
	}

	Ref<Pipeline> VulkanContext::GetDefaultPipelineForShader(Ref<Shader> shader, Ref<VulkanRenderPass> renderPass)
	{
		uint64_t key = (uint64_t)shader.get();
		auto it = m_DefaultPipelines.find(key);

		if (it != m_DefaultPipelines.end())
		{
			Ref<VulkanPipeline> pipeline = As<VulkanPipeline>(it->second);
			if (pipeline->GetCompatibleRenderPass().get() == renderPass.get())
			{
				return it->second;
			}
		}

		Ref<const ShaderMetadata> metadata = shader->GetMetadata();

		PipelineSpecifications specifications{};
		specifications.Shader = shader;
		specifications.DepthBiasEnabled = metadata->Features.DepthBiasEnabled;
		specifications.Culling = metadata->Features.Culling;
		specifications.DepthTest = metadata->Features.DepthTesting;
		specifications.DepthWrite = metadata->Features.DepthWrite;
		specifications.DepthClampEnabled = metadata->Features.DepthClampEnabled;
		specifications.DepthFunction = metadata->Features.DepthFunction;
		specifications.Blending = metadata->Features.Blending;

		// TODO: Sould be extracted by the ShaderCompiler from shader metadata
		specifications.DepthBiasSlopeFactor = 1.0f;
		specifications.DepthBiasConstantFactor = 0.5f;

		switch (metadata->Type)
		{
		case ShaderType::_2D:
		{
			specifications.InputLayout = PipelineInputLayout({
				{ 0, 0, ShaderDataType::Float3 }, // Position
				{ 0, 1, ShaderDataType::Float4 }, // Color
				{ 0, 2, ShaderDataType::Float2 }, // UV
				{ 0, 3, ShaderDataType::Int }, // Texture index
				{ 0, 4, ShaderDataType::Int }, // Entity index
			});
			break;
		}
		case ShaderType::Surface:
		{
			specifications.InputLayout = PipelineInputLayout({
				{ 0, 0, ShaderDataType::Float3 }, // Position
				{ 1, 1, ShaderDataType::Float3 }, // Normal
				{ 2, 2, ShaderDataType::Float3 }, // Tangent
				{ 3, 3, ShaderDataType::Float2 }, // UV
			});
			break;
		}
		case ShaderType::FullscreenQuad:
		{
			specifications.InputLayout = PipelineInputLayout({
				{ 0, 0, ShaderDataType::Float3 }, // Position
				{ 1, 1, ShaderDataType::Float3 }, // Normal
				{ 2, 2, ShaderDataType::Float3 }, // Tangent
				{ 3, 3, ShaderDataType::Float2 }, // UV
			});
			break;
		}
		case ShaderType::Decal:
		{
			specifications.InputLayout = PipelineInputLayout({
				{ 0, 0, ShaderDataType::Float3 }, // Position
			});
			break;
		}
		default:
			FLARE_CORE_ASSERT(false);
		}

		Ref<Pipeline> pipeline = As<Pipeline>(CreateRef<VulkanPipeline>(specifications, renderPass));
		m_DefaultPipelines.emplace(key, pipeline);
		return pipeline;
	}

	VulkanRenderPassCache& VulkanContext::GetRenderPassCache()
	{
		return m_RenderPassCache;
	}

	void VulkanContext::GetSourcePipelineStagesAndAccessFlags(VkImageLayout imageLayout, VkPipelineStageFlags& stages, VkAccessFlags& access)
	{
		switch (imageLayout)
		{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			stages |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			access |= VK_ACCESS_NONE;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// NOTE: Includes all posible stages that could read an image.
			//       It might not give the most optimal result in some specific situations
			stages |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
				| VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			access |= VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_GENERAL:
			// NOTE: Same thing here, includes all possible stages & also access flags
			stages |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
				| VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			access |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			access |= VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			access |= VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		default:
			FLARE_CORE_ASSERT(false);
			break;
		}
	}

	void VulkanContext::GetDestinationPipelineStagesAndAccessFlags(VkImageLayout imageLayout, VkPipelineStageFlags& stages, VkAccessFlags& access)
	{
		FLARE_CORE_ASSERT(imageLayout != VK_IMAGE_LAYOUT_UNDEFINED);
		switch (imageLayout)
		{
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			// NOTE: Includes all posible stages that could read an image.
			//       It might not give the most optimal result in some specific situations
			stages |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
				| VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			access |= VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_GENERAL:
			stages |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
				| VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				| VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
			access |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			stages |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			access |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			stages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			access |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			access |= VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
			access |= VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			stages |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			access |= VK_ACCESS_NONE;
			break;
		default:
			FLARE_CORE_ERROR("{}", (int64_t)imageLayout);
			FLARE_CORE_ASSERT(false);
			break;
		}
	}

	void VulkanContext::FillPipelineStagesAndAccessMasks(VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkPipelineStageFlags& sourceStages,
		VkPipelineStageFlags& destinationStages,
		VkAccessFlags& sourceAccess,
		VkAccessFlags& destinationAccess)
	{
		GetSourcePipelineStagesAndAccessFlags(oldLayout, sourceStages, sourceAccess);
		GetDestinationPipelineStagesAndAccessFlags(newLayout, destinationStages, destinationAccess);
	}

	void VulkanContext::CreateInstance(const Span<const char*>& enabledLayers)
	{
		FLARE_PROFILE_FUNCTION();
		std::vector<const char*> instanceExtensions;

		{
			uint32_t count = 0;
			const char** extesions = glfwGetRequiredInstanceExtensions(&count);

			for (uint32_t i = 0; i < count; i++)
				instanceExtensions.push_back(extesions[i]);
		}

		if (m_DebugEnabled)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		VkApplicationInfo applicationInfo{};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.apiVersion = VK_API_VERSION_1_3;
		applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.pApplicationName = "Flare";
		applicationInfo.pEngineName = "Flare";

		VkInstanceCreateInfo instanceInfo{};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceInfo.ppEnabledExtensionNames = instanceExtensions.data();
		instanceInfo.enabledLayerCount = (uint32_t)enabledLayers.GetSize();
		instanceInfo.ppEnabledLayerNames = enabledLayers.GetData();

		VK_CHECK_RESULT(vkCreateInstance(&instanceInfo, nullptr, &m_Instance));
	}

	void VulkanContext::CreateDebugMessenger()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_DebugEnabled);

		m_CreateDebugMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));
		m_DestroyDebugMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));

		FLARE_CORE_ASSERT(m_CreateDebugMessenger && m_DestroyDebugMessenger);

		VkDebugUtilsMessengerCreateInfoEXT info{};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		info.pNext = nullptr;
		info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		info.flags = 0;
		info.pfnUserCallback = VulkanDebugCallback;

		VK_CHECK_RESULT(m_CreateDebugMessenger(m_Instance, &info, nullptr, &m_DebugMessenger));
	}

	void VulkanContext::CreateSurface()
	{
		FLARE_PROFILE_FUNCTION();
		VK_CHECK_RESULT(glfwCreateWindowSurface(m_Instance, (GLFWwindow*)m_Window->GetNativeWindow(), nullptr, &m_Surface));
	}

	void VulkanContext::ChoosePhysicalDevice(VkPhysicalDeviceType deviceType)
	{
		FLARE_PROFILE_FUNCTION();
		uint32_t physicalDevicesCount;
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, nullptr));

		std::vector<VkPhysicalDevice> devices(physicalDevicesCount);
		VK_CHECK_RESULT(vkEnumeratePhysicalDevices(m_Instance, &physicalDevicesCount, devices.data()));

		for (VkPhysicalDevice device : devices)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(device, &properties);

			if (properties.deviceType == deviceType)
			{
				m_PhysicalDevice = device;
			}
		}

		FLARE_CORE_ASSERT(m_PhysicalDevice);
	}

	void VulkanContext::GetQueueFamilyProperties()
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t count;
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, nullptr);

		std::vector<VkQueueFamilyProperties> properties(count);
		vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &count, properties.data());

		for (uint32_t i = 0; i < count; i++)
		{
			if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				m_GraphicsQueueFamilyIndex = i;

			VkBool32 supported = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &supported);

			if (supported)
				m_PresentQueueFamilyIndex = i;
		}

		FLARE_CORE_ASSERT(m_GraphicsQueueFamilyIndex.has_value() && m_PresentQueueFamilyIndex.has_value());
	}

	void VulkanContext::CreateLogicalDevice(const Span<const char*>& enabledLayers, const Span<const char*>& enabledExtensions)
	{
		FLARE_PROFILE_FUNCTION();
		float priority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> createInfos;

		if (*m_GraphicsQueueFamilyIndex == *m_PresentQueueFamilyIndex)
		{
			VkDeviceQueueCreateInfo& queueCreateInfo = createInfos.emplace_back();
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = *m_GraphicsQueueFamilyIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;
		}
		else
		{
			VkDeviceQueueCreateInfo& queueCreateInfo = createInfos.emplace_back();
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = *m_GraphicsQueueFamilyIndex;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &priority;

			VkDeviceQueueCreateInfo& presentationQueueCreateInfo = createInfos.emplace_back();
			presentationQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			presentationQueueCreateInfo.queueFamilyIndex = *m_PresentQueueFamilyIndex;
			presentationQueueCreateInfo.queueCount = 1;
			presentationQueueCreateInfo.pQueuePriorities = &priority;
		}

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.depthClamp = VK_TRUE;

		VkPhysicalDeviceSynchronization2Features synchronization2{};
		synchronization2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
		synchronization2.synchronization2 = true;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pQueueCreateInfos = createInfos.data();
		deviceCreateInfo.queueCreateInfoCount = (uint32_t)createInfos.size();
		deviceCreateInfo.pNext = &synchronization2;

		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

		deviceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.GetSize();
		deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.GetData();

		deviceCreateInfo.enabledLayerCount = (uint32_t)enabledLayers.GetSize();
		deviceCreateInfo.ppEnabledLayerNames = enabledLayers.GetData();

		VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device));

		vkGetDeviceQueue(m_Device, *m_GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, *m_PresentQueueFamilyIndex, 0, &m_PresentQueue);
	}

	void VulkanContext::CreateMemoryAllocator()
	{
		FLARE_PROFILE_FUNCTION();
		VmaAllocatorCreateInfo info{};
		info.instance = m_Instance;
		info.device = m_Device;
		info.physicalDevice = m_PhysicalDevice;
		info.vulkanApiVersion = VK_API_VERSION_1_3;
		info.flags = VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;

		VK_CHECK_RESULT(vmaCreateAllocator(&info, &m_Allocator));
	}

	void VulkanContext::CreateCommandBufferPool()
	{
		FLARE_PROFILE_FUNCTION();
		VkCommandPoolCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		createInfo.queueFamilyIndex = *m_GraphicsQueueFamilyIndex;

		VK_CHECK_RESULT(vkCreateCommandPool(m_Device, &createInfo, nullptr, &m_CommandBufferPool));
	}

	VkCommandBuffer VulkanContext::CreateCommandBuffer()
	{
		FLARE_PROFILE_FUNCTION();
		VkCommandBufferAllocateInfo allocation{};
		allocation.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocation.commandBufferCount = 1;
		allocation.commandPool = m_CommandBufferPool;
		allocation.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer commandBuffer;
		VK_CHECK_RESULT(vkAllocateCommandBuffers(m_Device, &allocation, &commandBuffer));
		return commandBuffer;
	}

	void VulkanContext::CreateSyncObjects()
	{
		FLARE_PROFILE_FUNCTION();
		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		m_SyncObjects.resize(m_Swapchain->GetFrameCount());

		for (uint32_t i = 0; i < m_Swapchain->GetFrameCount(); i++)
		{
			VK_CHECK_RESULT(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_SyncObjects[i].RenderingCompleteSemaphore));
			VK_CHECK_RESULT(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_SyncObjects[i].RenderingCompleteSemaphore2));
			VK_CHECK_RESULT(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_SyncObjects[i].FrameFence));
		}
	}

	std::vector<VkLayerProperties> VulkanContext::EnumerateAvailableLayers()
	{
		FLARE_PROFILE_FUNCTION();
		uint32_t supportedLayersCount = 0;
		std::vector<VkLayerProperties> supportedLayers;

		VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&supportedLayersCount, nullptr));

		supportedLayers.resize(supportedLayersCount);

		VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&supportedLayersCount, supportedLayers.data()));

		return supportedLayers;
	}

	VkImageLayout ImageLayoutToVulkanImageLayout(ImageLayout layout, TextureFormat format)
	{
		switch (layout)
		{
		case ImageLayout::Undefined:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ImageLayout::General:
			return VK_IMAGE_LAYOUT_GENERAL;
		case ImageLayout::ReadOnly:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ImageLayout::TransferSource:
			return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		case ImageLayout::TransferDestination:
			return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		case ImageLayout::AttachmentOutput:
		{
			if (IsDepthTextureFormat(format))
			{
				//if (HasStencilComponent(format))
				{
					return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}

				return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			}

			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		}

		FLARE_CORE_ASSERT(false);
		return VK_IMAGE_LAYOUT_UNDEFINED;
	}

	FLARE_API VkCompareOp DepthComparisonFunctionToVulkanCompareOp(DepthComparisonFunction function)
	{
		VkCompareOp depthComapreFunction = VK_COMPARE_OP_NEVER;
		switch (function)
		{
		case DepthComparisonFunction::Less:
			depthComapreFunction = VK_COMPARE_OP_LESS;
			break;
		case DepthComparisonFunction::LessOrEqual:
			depthComapreFunction = VK_COMPARE_OP_LESS_OR_EQUAL;
			break;
		case DepthComparisonFunction::Always:
			depthComapreFunction = VK_COMPARE_OP_ALWAYS;
			break;
		case DepthComparisonFunction::Never:
			depthComapreFunction = VK_COMPARE_OP_NEVER;
			break;
		case DepthComparisonFunction::Greater:
			depthComapreFunction = VK_COMPARE_OP_GREATER;
			break;
		case DepthComparisonFunction::GreaterOrEqual:
			depthComapreFunction = VK_COMPARE_OP_GREATER_OR_EQUAL;
			break;
		case DepthComparisonFunction::Equal:
			depthComapreFunction = VK_COMPARE_OP_EQUAL;
			break;
		case DepthComparisonFunction::NotEqual:
			depthComapreFunction = VK_COMPARE_OP_NOT_EQUAL;
			break;
		default:
			FLARE_CORE_ASSERT(false);
		}

		return depthComapreFunction;
	}
}
