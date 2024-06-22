#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"
#include "Flare/Platform/Vulkan/VulkanAllocation.h"

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
	struct StagingBuffer
	{
		VkBuffer Buffer = VK_NULL_HANDLE;
		VulkanAllocation Allocation;
	};

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
		void Present() override;
		void WaitForDevice() override;

		bool IsValid() const { return m_Device != VK_NULL_HANDLE; }

		void CreateBuffer(size_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkBuffer& buffer, VkDeviceMemory& memory);
		VulkanAllocation CreateStagingBuffer(size_t size, VkBuffer& buffer);

		Ref<VulkanCommandBuffer> GetPrimaryCommandBuffer() const { return m_PrimaryCommandBuffer; }
		Ref<CommandBuffer> GetCommandBuffer() const override;

		uint32_t GetCurrentFrameInFlight() const { return m_CurrentFrameInFlight; }
		Ref<VulkanFrameBuffer> GetSwapChainFrameBuffer(uint32_t index) const { return m_SwapChainFrameBuffers[index]; }

		Ref<VulkanCommandBuffer> BeginTemporaryCommandBuffer();
		void EndTemporaryCommandBuffer(Ref<VulkanCommandBuffer> commandBuffer);

		VkInstance GetVulkanInstance() const { return m_Instance; }
		VkDevice GetDevice() const { return m_Device; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }

		Ref<VulkanRenderPass> FindOrCreateRenderPass(Span<TextureFormat> formats);

		VmaAllocator GetMemoryAllocator() const { return m_Allocator; }

		uint32_t GetGraphicsQueueFamilyIndex() const { return *m_GraphicsQueueFamilyIndex; }
		Ref<VulkanRenderPass> GetColorOnlyPass() const { return m_ColorOnlyPass; }

		void SetImageViewDeletionHandler(const std::function<void(VkImageView)>& handler) { m_ImageDeletationHandler = handler; }
		void NotifyImageViewDeletionHandler(VkImageView deletedImageView);

		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		VkResult SetDebugName(VkObjectType objectType, uint64_t objectHandle, const char* name);

		void DeferDestroyStagingBuffer(StagingBuffer stagingBuffer);

		Ref<Pipeline> GetDefaultPipelineForShader(Ref<Shader> shader, Ref<VulkanRenderPass> renderPass);

		static VulkanContext& GetInstance() { return *(VulkanContext*)&GraphicsContext::GetInstance(); }
	private:
		void CreateInstance(const Span<const char*>& enabledLayers);
		void CreateDebugMessenger();
		void CreateSurface();
		void ChoosePhysicalDevice();
		void GetQueueFamilyProperties();
		void CreateLogicalDevice(const Span<const char*>& enabledLayers, const Span<const char*>& enabledExtensions);

		void CreateMemoryAllocator();
	
		void CreateCommandBufferPool();
		VkCommandBuffer CreateCommandBuffer();
		void CreateSyncObjects();

		void CreateSwapChain();
		void RecreateSwapChain();
		void ReleaseSwapChainResources();
		void CreateSwapChainImageViews();
		void CreateSwapChainFrameBuffers();

		uint32_t ChooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& formats);
		VkExtent2D GetSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& modes);

		void DestroyStagingBuffers();
	private:
		std::vector<VkLayerProperties> EnumerateAvailableLayers();
	private:
		std::function<void(VkImageView)> m_ImageDeletationHandler = nullptr;

		bool m_DebugEnabled = false;
		uint32_t m_FramesInFlight = 0;
		uint32_t m_CurrentFrameInFlight = 0;

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

		// Window
		bool m_VSyncEnabled = false;
		Ref<Window> m_Window = nullptr;

		// Swap chain
		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;

		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		std::vector<Ref<VulkanFrameBuffer>> m_SwapChainFrameBuffers;

		glm::uvec2 m_SwapChainExtent = glm::uvec2(0);
		VkFormat m_SwapChainImageFormat = VK_FORMAT_UNDEFINED;

		VkFence m_FrameFence = VK_NULL_HANDLE;
		VkSemaphore m_ImageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore m_RenderFinishedSemaphore = VK_NULL_HANDLE;

		// Command buffers
		VkCommandPool m_CommandBufferPool = VK_NULL_HANDLE;
		Ref<VulkanCommandBuffer> m_PrimaryCommandBuffer = nullptr;

		// Render pass
		Ref<VulkanRenderPass> m_ColorOnlyPass = nullptr;
		std::unordered_map<RenderPassKey, Ref<VulkanRenderPass>> m_RenderPasses;

		std::unordered_map<uint64_t, Ref<Pipeline>> m_DefaultPipelines;

		// Allocator
		VmaAllocator m_Allocator = VK_NULL_HANDLE;

		// Stating buffers
		std::vector<StagingBuffer> m_CurrentFrameStagingBuffers;
	};
}
