#pragma once

#include "Flare/Renderer/Pipeline.h"

#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	class Material;
	class VulkanPipeline;
	class VulkanRenderPass;

	class FLARE_API VulkanCommandBuffer
	{
	public:
		VulkanCommandBuffer(VkCommandBuffer commandBuffer);

		void Reset();

		void Begin();
		void End();

		void BeginRenderPass(const Ref<VulkanRenderPass>& renderPass, const Ref<VulkanFrameBuffer>& frameBuffer);
		void EndRenderPass();

		void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void TransitionDepthImageLayout(VkImage image, bool hasStencilComponent, VkImageLayout oldLayout, VkImageLayout newLayout);

		void ClearImage(VkImage image, const glm::vec4& clearColor, VkImageLayout oldLayout, VkImageLayout newLayout);
		void ClearDepthStencilImage(VkImage image, bool hasStencilComponent, float depthValue, uint32_t stencilValue, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D size);
		void CopyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, size_t size, size_t sourceOffset, size_t destinationOffset);

		void BindPipeline(const Ref<const Pipeline>& pipeline);
		void BindVertexBuffers(const Span<Ref<const VertexBuffer>>& vertexBuffers);
		void BindIndexBuffer(const Ref<const IndexBuffer>& indexBuffer);

		void BindDescriptorSet(const Ref<const VulkanDescriptorSet>& descriptorSet, VkPipelineLayout pipelineLayout, uint32_t index);

		void SetPrimaryDescriptorSet(const Ref<VulkanDescriptorSet>& set);
		void SetSecondaryDescriptorSet(const Ref<VulkanDescriptorSet>& set);
		void ApplyMaterial(const Ref<const Material>& material);

		void SetViewportAndScisors(Math::Rect viewportRect);

		void DrawIndexed(uint32_t indicesCount);
		void DrawIndexed(uint32_t firstIndex, uint32_t indicesCount);
		void DrawIndexed(uint32_t firstIndex, uint32_t indicesCount, uint32_t firstInstance, uint32_t instancesCount);
		void DrawIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t firstInstance, uint32_t instancesCount);

		VkCommandBuffer GetHandle() const { return m_CommandBuffer; }
	private:
		Ref<VulkanDescriptorSet> m_PrimaryDescriptorSet = nullptr;
		Ref<VulkanDescriptorSet> m_SecondaryDescriptorSet = nullptr;

		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
		Ref<VulkanRenderPass> m_CurrentRenderPass = nullptr;
		Ref<const VulkanPipeline> m_CurrentPipeline = nullptr;
	};
}
