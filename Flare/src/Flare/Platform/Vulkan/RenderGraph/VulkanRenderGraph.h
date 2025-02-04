#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

#include "Flare/Platform/Vulkan/RenderGraph/TextureViewsManager.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class LayoutTransitionsGenerator;
	class VulkanRenderPass;
	class VulkanFrameBuffer;

	class FLARE_API VulkanRenderTarget
	{
	public:
		VulkanRenderTarget() = default;
		VulkanRenderTarget(glm::uvec2 size, Span<const VkImageView> attachments, Ref<VulkanRenderPass> compatibleRenderPass, const char* debugName);

		VulkanRenderTarget(const VulkanRenderTarget& other);
		VulkanRenderTarget(VulkanRenderTarget&& other) noexcept;

		~VulkanRenderTarget();

		VulkanRenderTarget& operator=(VulkanRenderTarget&& other) noexcept;
		VulkanRenderTarget& operator=(const VulkanRenderTarget& other);

		inline bool IsCreated() const { return m_Handle != VK_NULL_HANDLE; }

		inline VkFramebuffer GetHandle() const { return m_Handle; }
		inline glm::uvec2 GetSize() const { return m_Size; }
		inline Ref<VulkanRenderPass> GetCompatibleRenderPass() const { return m_CompatibleRenderPass; }
	private:
		void Release();
	private:
		std::string m_DebugName;
		glm::uvec2 m_Size = glm::uvec2(0, 0);
		VkFramebuffer m_Handle = VK_NULL_HANDLE;
		Ref<VulkanRenderPass> m_CompatibleRenderPass = nullptr;
	};

	class VulkanCommandBuffer;
	class FLARE_API VulkanRenderGraph : public RenderGraph
	{
	public:
		VulkanRenderGraph(World& renderWorld, Entity viewportEntity);

		void ExecuteRenderPasses(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view) override;
	private:
		size_t GetRenderTargetIndex(size_t nodeIndex) const;

		void ExecuteLayoutTransitions(Ref<CommandBuffer> commandBuffer, LayoutTransitionsRange range);
		void CreateRenderTargets(uint32_t frameIndex);

		void SelectVulkanRenderPasses(const LayoutTransitionsGenerator& renderGraphBuilder);
		void FillClearValuesBuffer();

		void CreateRenderTargetForNode(size_t nodeIndex, uint32_t frameIndex, std::vector<VkImageView>& temporaryAttachmentsStorage);
		void ClearExternalTexture(const ExternalRenderGraphResource& externalTexture, VulkanCommandBuffer& commandBuffer) const;
	protected:
		void OnPrepare() override;
		void OnTexturesResize() override;
		void OnClear() override;
		void OnBuild() override;
		void OnAfterAllocatingOnDemandTextures(const std::unordered_set<RenderGraphTextureId>& updatedTextures) override;
	private:
		struct ClearValuesRange
		{
			uint32_t Start = 0;
			uint32_t Count = 0;
		};

		struct NodeData
		{
			static constexpr uint32_t INVALID_TARGET_INDEX = UINT32_MAX;

			LayoutTransitionsRange ExplicitTransitions;
			uint32_t RenderTargetHandleIndex = INVALID_TARGET_INDEX;

			ClearValuesRange ClearValues;

			Ref<VulkanRenderPass> VulkanRenderPassHandle = nullptr;
		};

		TextureViewsManager m_TextureViews;

		std::vector<VulkanRenderTarget> m_RenderTargets;
		std::vector<NodeData> m_NodeData;
		std::vector<VkClearValue> m_ClearValuesBuffer;
	};
}
