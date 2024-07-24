#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/ShaderMetadata.h"

#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"
#include "Flare/Platform/Vulkan/VulkanAllocation.h"
#include "Flare/Platform/Vulkan/VulkanRenderPassCache.h"
#include "Flare/Platform/Vulkan/VulkanStagingBufferPool.h"
#include "Flare/Platform/Vulkan/VulkanSwapchain.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <optional>
#include <functional>
#include <unordered_map>

#define VK_CHECK_RESULT(expression) {                       \
		VkResult __result = (expression);                   \
		if (__result != VK_SUCCESS)                         \
		{                                                   \
			FLARE_CORE_ERROR("'{}' failed with result: {}", \
			#expression,                                    \
			(std::underlying_type_t<VkResult>)__result);    \
			FLARE_CORE_ASSERT(false);                       \
		}                                                   \
	}

namespace Flare
{
	struct RenderPassKey
	{
		bool operator==(const RenderPassKey& other) const
		{
			if (other.Formats.size() != Formats.size())
				return false;

			for (size_t i = 0; i < Formats.size(); i++)
			{
				if (other.Formats[i] != Formats[i])
					return false;
			}

			return true;
		}

		bool operator!=(const RenderPassKey& other) const
		{
			return !operator==(other);
		}

		std::vector<TextureFormat> Formats;
	};
}

template<>
struct std::hash<Flare::RenderPassKey>
{
	size_t operator()(const Flare::RenderPassKey& key) const
	{
		using FormatIntType = std::underlying_type_t<Flare::TextureFormat>;
		size_t hash = std::hash<FormatIntType>()((FormatIntType)key.Formats[0]);

		for (size_t i = 0; i < key.Formats.size(); i++)
		{
			Flare::CombineHashes(hash, (FormatIntType)key.Formats[i]);
		}

		return hash;
	}
};

namespace Flare
{
	class DescriptorSet;
	class DescriptorSetLayout;
	class DescriptorSetPool;
	class Shader;

	class VulkanCommandBuffer;
	class VulkanFrameBuffer;

	FLARE_API VkImageLayout ImageLayoutToVulkanImageLayout(ImageLayout layout, TextureFormat format);
	FLARE_API VkCompareOp DepthComparisonFunctionToVulkanCompareOp(DepthComparisonFunction function);

	class FLARE_API VulkanContext : public GraphicsContext
	{
	public:
		VulkanContext(Ref<Window> window);
		~VulkanContext();

		void Initialize() override;
		void Release() override;
		void BeginFrame() override;
		void EndFrame() override;
		void Present() override;
		void WaitForDevice() override;

		Ref<CommandBuffer> GetCommandBuffer() const override;

		bool IsValid() const { return m_Device != VK_NULL_HANDLE; }

		Ref<VulkanCommandBuffer> GetPrimaryCommandBuffer() const { return m_PrimaryCommandBuffer; }

		void SubmitToGraphicsQueue(Ref<CommandBuffer> commandBuffer,
			Span<const VkSemaphore> waitSempahores,
			Span<const VkSemaphore> signalSemaphores,
			bool waitForMainRenderingSubmition);

		void SubmitSwapchainPresent(VulkanSwapchain& swapchain,
			Span<const VkSemaphore> waitSemaphores,
			bool waitForMainRenderingSubmition);

		uint32_t GetCurrentFrameInFlight() const { return m_Swapchain->GetFrameInFlight(); }
		uint32_t GetFrameInFlightCount() const { return m_Swapchain->GetFrameCount(); }
		Ref<VulkanFrameBuffer> GetSwapChainFrameBuffer(uint32_t index) const { return m_Swapchain->GetFrameBuffer(index); }

		Ref<VulkanCommandBuffer> BeginTemporaryCommandBuffer();
		void EndTemporaryCommandBuffer(Ref<VulkanCommandBuffer> commandBuffer);

		VkInstance GetVulkanInstance() const { return m_Instance; }
		VkDevice GetDevice() const { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() const { return m_PresentQueue; }

		Ref<VulkanRenderPass> FindOrCreateRenderPass(Span<TextureFormat> formats);

		VmaAllocator GetMemoryAllocator() const { return m_Allocator; }

		uint32_t GetGraphicsQueueFamilyIndex() const { return *m_GraphicsQueueFamilyIndex; }
		uint32_t GetPresentQueueFamilyIndex() const { return *m_PresentQueueFamilyIndex; }
		Ref<VulkanRenderPass> GetColorOnlyPass() const { return m_ColorOnlyPass; }

		void SetImageViewDeletionHandler(const std::function<void(VkImageView)>& handler) { m_ImageDeletationHandler = handler; }
		void NotifyImageViewDeletionHandler(VkImageView deletedImageView);

		VkResult SetDebugName(VkObjectType objectType, uint64_t objectHandle, const char* name);

		Ref<Pipeline> GetDefaultPipelineForShader(Ref<Shader> shader, Ref<VulkanRenderPass> renderPass);

		VulkanRenderPassCache& GetRenderPassCache();
		VulkanStagingBufferPool& GetStagingBufferPool() { return m_StagingBufferPool; }
		const VulkanStagingBufferPool& GetStagingBufferPool() const { return m_StagingBufferPool; }

		Ref<DescriptorSet> GetEmptyDescriptorSet() const { return m_EmptyDescriptorSet; }
		Ref<DescriptorSetLayout> GetEmptyDescriptorSetLayout() const { return m_EmptyDescriptorSetLayout; }

		// Gets the pipeline stages and corrisponding access flags when transitioning from a given image layout
		static void GetSourcePipelineStagesAndAccessFlags(VkImageLayout imageLayout, VkPipelineStageFlags& stages, VkAccessFlags& access);

		// Gets the pipeline stages and corrisponding access flags when transitioning to a given image layout
		static void GetDestinationPipelineStagesAndAccessFlags(VkImageLayout imageLayout, VkPipelineStageFlags& stages, VkAccessFlags& access);

		static void FillPipelineStagesAndAccessMasks(VkImageLayout oldLayout,
			VkImageLayout newLayout,
			VkPipelineStageFlags& sourceStages,
			VkPipelineStageFlags& destinationStages,
			VkAccessFlags& sourceAccess,
			VkAccessFlags& destinationAccess);

		static VulkanContext& GetInstance() { return *(VulkanContext*)&GraphicsContext::GetInstance(); }
	private:
		void CreateInstance(const Span<const char*>& enabledLayers);
		void CreateDebugMessenger();
		void CreateSurface();
		void ChoosePhysicalDevice(VkPhysicalDeviceType deviceType);
		void GetQueueFamilyProperties();
		void CreateLogicalDevice(const Span<const char*>& enabledLayers, const Span<const char*>& enabledExtensions);

		void CreateMemoryAllocator();
	
		void CreateCommandBufferPool();
		VkCommandBuffer CreateCommandBuffer();
		void CreateSyncObjects();

		VkSemaphore AcquireSemaphore();
	private:
		std::vector<VkLayerProperties> EnumerateAvailableLayers();
	private:
		struct FrameSyncObjects
		{
			VkFence FrameFence = VK_NULL_HANDLE;
			std::vector<VkSemaphore> RenderingCompleteSemaphores;
		};

		struct GraphicsQueueSubmition
		{
			Ref<CommandBuffer> CommandBuffer = nullptr;
			uint32_t FirstWaitSemaphore = UINT32_MAX;
			uint32_t WaitSemaphoreCount = UINT32_MAX;
			uint32_t FirstSignalSemaphore = UINT32_MAX;
			uint32_t SignalSemaphoreCount = UINT32_MAX;
		};

		struct PresentSubmition
		{
			PresentSubmition(VulkanSwapchain& swapchain, uint32_t firstWaitSemaphore, uint32_t waitSemaphoreCount)
				: Swapchain(swapchain),
				SwapchainHandle(swapchain.GetHandle()),
				ImageIndex(swapchain.GetFrameInFlight()),
				FirstWaitSemaphore(firstWaitSemaphore),
				WaitSemaphoreCount(waitSemaphoreCount) {}

			VulkanSwapchain& Swapchain;
			VkSwapchainKHR SwapchainHandle = VK_NULL_HANDLE;
			uint32_t ImageIndex = UINT32_MAX;
			uint32_t FirstWaitSemaphore = UINT32_MAX;
			uint32_t WaitSemaphoreCount = UINT32_MAX;
		};

		std::vector<VkSemaphore> m_UsedSemaphores;
		std::vector<GraphicsQueueSubmition> m_GraphicsQueueSubmitions;
		std::vector<PresentSubmition> m_PresentSubmitions;

		std::function<void(VkImageView)> m_ImageDeletationHandler = nullptr;

		bool m_DebugEnabled = false;

		PFN_vkCreateDebugUtilsMessengerEXT m_CreateDebugMessenger = nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT m_DestroyDebugMessenger = nullptr;

		PFN_vkSetDebugUtilsObjectNameEXT m_SetDebugNameFunction = nullptr;

		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;

		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		VkInstance m_Instance = VK_NULL_HANDLE;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;

		std::optional<uint32_t> m_GraphicsQueueFamilyIndex;
		std::optional<uint32_t> m_PresentQueueFamilyIndex;

		// Empty descriptor
		Ref<DescriptorSetPool> m_EmptyDescriptorSetPool = nullptr;
		Ref<DescriptorSetLayout> m_EmptyDescriptorSetLayout = nullptr;
		Ref<DescriptorSet> m_EmptyDescriptorSet = nullptr;

		// Window
		bool m_VSyncEnabled = false;
		Ref<Window> m_Window = nullptr;

		// Syncronization
		std::vector<VkSemaphore> m_SemaphorePool;

		// Swap chain
		Scope<VulkanSwapchain> m_Swapchain;

		bool m_SkipWaitForFrameFence = false;

		uint32_t m_CurrentFrameSyncObjectsIndex = 0;
		FrameSyncObjects* m_CurrentSyncObjects = nullptr;
		std::vector<FrameSyncObjects> m_SyncObjects;

		// Command buffers
		VkCommandPool m_CommandBufferPool = VK_NULL_HANDLE;
		Ref<VulkanCommandBuffer> m_PrimaryCommandBuffer = nullptr;

		// Render pass
		Ref<VulkanRenderPass> m_ColorOnlyPass = nullptr;
		VulkanRenderPassCache m_RenderPassCache;
		std::unordered_map<RenderPassKey, Ref<VulkanRenderPass>> m_RenderPasses;

		std::unordered_map<uint64_t, Ref<Pipeline>> m_DefaultPipelines;

		// Allocator
		VmaAllocator m_Allocator = VK_NULL_HANDLE;

		// Staging buffers
		VulkanStagingBufferPool m_StagingBufferPool;
	};
}
