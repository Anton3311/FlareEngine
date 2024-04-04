#pragma once

#include "Flare/Renderer/Pipeline.h"
#include "Flare/Renderer/CommandBuffer.h"

#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"

#include <vulkan/vulkan.h>

#include <unordered_set>

namespace Flare
{
	class Material;
	class VulkanPipeline;
	class VulkanRenderPass;
	class VulkanGPUTimer;

	class FLARE_API VulkanCommandBuffer : public CommandBuffer
	{
	public:
		VulkanCommandBuffer(VkCommandBuffer commandBuffer);

		void BeginRenderTarget(const Ref<FrameBuffer> frameBuffer) override;
		void EndRenderTarget() override;

		void ApplyMaterial(const Ref<const Material>& material) override;

		void SetViewportAndScisors(Math::Rect viewportRect) override;

		void DrawIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t baseInstance, uint32_t instanceCount) override;

		void StartTimer(Ref<GPUTimer> timer) override;
		void StopTimer(Ref<GPUTimer> timer) override;
	public:
		void Reset();

		void Begin();
		void End();

		void BeginRenderPass(const Ref<VulkanRenderPass>& renderPass, const Ref<VulkanFrameBuffer>& frameBuffer);
		void EndRenderPass();

		void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t mipLevels);
		void TransitionDepthImageLayout(VkImage image, bool hasStencilComponent, VkImageLayout oldLayout, VkImageLayout newLayout);

		void ClearImage(VkImage image, const glm::vec4& clearColor, VkImageLayout oldLayout, VkImageLayout newLayout);
		void ClearDepthStencilImage(VkImage image, bool hasStencilComponent, float depthValue, uint32_t stencilValue, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D size);
		void CopyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, size_t size, size_t sourceOffset, size_t destinationOffset);

		void GenerateImageMipMaps(VkImage image, uint32_t mipLevels, glm::uvec2 imageSize);

		void BindPipeline(const Ref<const Pipeline>& pipeline);
		void BindVertexBuffers(const Span<Ref<const VertexBuffer>>& vertexBuffers);
		void BindIndexBuffer(const Ref<const IndexBuffer>& indexBuffer);

		void BindDescriptorSet(const Ref<const VulkanDescriptorSet>& descriptorSet, VkPipelineLayout pipelineLayout, uint32_t index);

		void SetPrimaryDescriptorSet(const Ref<VulkanDescriptorSet>& set);
		void SetSecondaryDescriptorSet(const Ref<VulkanDescriptorSet>& set);

		void DrawIndexed(uint32_t indicesCount);
		void DrawIndexed(uint32_t firstIndex, uint32_t indicesCount);
		void DrawIndexed(uint32_t firstIndex, uint32_t indicesCount, uint32_t firstInstance, uint32_t instancesCount);

		void DepthImagesBarrier(Span<VkImage> images, bool hasStencil,
			VkPipelineStageFlags srcStage, VkAccessFlags srcAccessMask,
			VkPipelineStageFlags dstStage, VkAccessFlags dstAccessMask,
			VkImageLayout oldLayout, VkImageLayout newLayout);

		void BeginTimer(Ref<VulkanGPUTimer> timer, VkPipelineStageFlagBits pipelineStages);
		void EndTimer(Ref<VulkanGPUTimer> timer, VkPipelineStageFlagBits pipelineStages);

		VkCommandBuffer GetHandle() const { return m_CommandBuffer; }
	private:
		std::unordered_set<uint64_t> m_UsedMaterials;
		std::vector<VkImageMemoryBarrier> m_ImageBarriers;

		Ref<VulkanDescriptorSet> m_PrimaryDescriptorSet = nullptr;
		Ref<VulkanDescriptorSet> m_SecondaryDescriptorSet = nullptr;

		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
		Ref<VulkanRenderPass> m_CurrentRenderPass = nullptr;
		Ref<const VulkanPipeline> m_CurrentPipeline = nullptr;
	};
}
