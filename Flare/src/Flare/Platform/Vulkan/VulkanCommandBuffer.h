#pragma once

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/CommandBuffer.h"

#include <vulkan/vulkan.h>

#include <unordered_set>

namespace Flare
{
	class Material;

	class VulkanDescriptorSet;
	class VulkanGPUTimer;
	class VulkanPipeline;
	class VulkanRenderPass;

	enum class MeshType
	{
		Default,
		DepthOnly,
	};

	class FLARE_API VulkanCommandBuffer : public CommandBuffer
	{
	public:
		VulkanCommandBuffer(VkCommandBuffer commandBuffer);

		void BeginLabel(const glm::vec4& color, const std::string& label) override;
		void EndLabel() override;

		void ClearColor(const Ref<Texture>& texture, const glm::vec4& clearColor) override;
		void ClearDepth(const Ref<Texture>& texture, float depth) override;

		void ApplyMaterial(const Ref<const Material>& material) override;

		void PushDescriptorProperties(ShaderDescriptorBuffer& descriptorProperties) override;
		void PushConstants(const ShaderConstantBuffer& constantBuffer) override;

		void SetViewportAndScissors(Math::Rect viewportRect) override;
		void SetDefaultViewportAndScissors() override;

		void BindPipeline(const Ref<Pipeline>& pipeline) override;
		void BindVertexBuffer(Ref<const GPUBuffer> buffer, uint32_t index) override;
		void BindVertexBuffers(Span<Ref<const GPUBuffer>> vertexBuffers, uint32_t baseBindingIndex) override;
		void BindIndexBuffer(Ref<const GPUBuffer> buffer, IndexFormat format) override;

		void DrawMeshIndexed(const Ref<const Mesh>& mesh, uint32_t baseInstance, uint32_t instanceCount) override;

		void DrawDepthOnlyMeshIndexed(const Ref<const Mesh>& mesh,
				uint32_t baseInstance,
				uint32_t instanceCount) override;

		void DrawDepthOnlyMeshIndexed(const Ref<const Mesh>& mesh,
				uint32_t subMeshIndex,
				uint32_t baseInstance,
				uint32_t instanceCount) override;

		void DrawMeshIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t baseInstance, uint32_t instanceCount) override;
		void DrawMeshIndexed(const Ref<const Mesh>& mesh, uint32_t firstSubMesh, uint32_t subMeshCount, uint32_t baseInstance, uint32_t instanceCount);

		void DrawIndexed(uint32_t baseIndex,
			uint32_t indexCount,
			uint32_t vertexOffset,
			uint32_t baseInstance,
			uint32_t instanceCount) override;

		void Draw(uint32_t baseVertex,
			uint32_t vertexCount,
			uint32_t baseInstance,
			uint32_t instanceCount) override;

		// Expects source attachment to be in TRANSFER_SRC
		// Destination attachment - TRANSFER_DST
		// Leaves layouts unchanged
		void Blit(Ref<const Texture> source, Ref<const Texture> destination, TextureFiltering filter) override;

		void SetGlobalDescriptorSet(Ref<const DescriptorSet> set, uint32_t index) override;

		void BindComputeShader(Ref<ComputeShader> computeShader) override;
		void DispatchCompute(const glm::uvec3& groupCount) override;

		void StartTimer(Ref<GPUTimer> timer) override;
		void StopTimer(Ref<GPUTimer> timer) override;
	public:
		void Reset();
		void ResetBoundPipelineState();
		void ResetCurrentDescriptorSets();

		void RebindGlobalDescriptorSets();

		void Begin();
		void End();

		void BeginRenderPass(VkFramebuffer frameBuffer,
			const Ref<VulkanRenderPass>& renderPass,
			glm::uvec2 renderAreaSize,
			Span<const VkClearValue> clearValues);
		void EndRenderPass();

		void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t mipLevels);
		void TransitionDepthImageLayout(VkImage image, bool hasStencilComponent, VkImageLayout oldLayout, VkImageLayout newLayout);

		void ClearImage(VkImage image, const glm::vec4& clearColor, VkImageLayout oldLayout, VkImageLayout newLayout);
		void ClearDepthStencilImage(VkImage image, bool hasStencilComponent, float depthValue, uint32_t stencilValue, VkImageLayout oldLayout, VkImageLayout newLayout);
		void CopyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D size, size_t bufferOffset, uint32_t mip);
		void CopyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, size_t size, size_t sourceOffset, size_t destinationOffset);

		void AddBufferBarrier(Span<VkBufferMemoryBarrier> memoryBarriers, VkPipelineStageFlags sourceStages, VkPipelineStageFlags destinationStages);

		void GenerateImageMipMaps(VkImage image, uint32_t mipLevels, glm::uvec2 imageSize);

		void BindDescriptorSet(Ref<const DescriptorSet> descriptorSet, uint32_t index);

		void BindDescriptorSet(const Ref<const VulkanDescriptorSet>& descriptorSet,
			VkPipelineLayout pipelineLayout,
			VkPipelineBindPoint bindPoint,
			uint32_t index);

		void BindMesh(const Ref<const Mesh>& mesh, MeshType meshType = MeshType::Default);

		void DepthImagesBarrier(Span<VkImage> images, bool hasStencil,
			VkPipelineStageFlags srcStage, VkAccessFlags srcAccessMask,
			VkPipelineStageFlags dstStage, VkAccessFlags dstAccessMask,
			VkImageLayout oldLayout, VkImageLayout newLayout);

		void BeginTimer(Ref<VulkanGPUTimer> timer, VkPipelineStageFlagBits pipelineStages);
		void EndTimer(Ref<VulkanGPUTimer> timer, VkPipelineStageFlagBits pipelineStages);

		VkCommandBuffer GetHandle() const { return m_CommandBuffer; }
	private:
		static constexpr size_t GLOBAL_DESCRIPTOR_SET_COUNT = 3;
		std::vector<VkImageMemoryBarrier> m_ImageBarriers;

		Ref<const VulkanDescriptorSet> m_GlobalDescriptorSets[GLOBAL_DESCRIPTOR_SET_COUNT] = { nullptr }; // Slot 3 is material resources
		bool m_GlobalDescriptorSetsRequireBinding = false;

		struct BoundPipelineState
		{
			VkPipelineBindPoint BindPoint = VK_PIPELINE_BIND_POINT_MAX_ENUM;
			VkPipelineLayout LayoutHandle = VK_NULL_HANDLE;
			VkPipeline PipelineHandle = VK_NULL_HANDLE;

			Ref<Pipeline> GraphicsPipeline = nullptr;
			Ref<ComputeShader> ComputeShader = nullptr;
		};

		struct BoundMeshState
		{
			Ref<const Mesh> Mesh = nullptr;
			MeshType Type = MeshType::Default;
		};

		struct BoundDescriptorSet
		{
			Ref<const VulkanDescriptorSet> Set = nullptr;
			VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		};

		struct RenderTargetState
		{
			inline bool IsValid() const
			{
				return FrameBufferHandle != VK_NULL_HANDLE && RenderPass != nullptr;
			}

			VkFramebuffer FrameBufferHandle = VK_NULL_HANDLE;
			glm::uvec2 RenderAreaSize = glm::uvec2(0, 0);
			Ref<VulkanRenderPass> RenderPass = nullptr;
		};

		BoundDescriptorSet m_CurrentDescriptorSets[4] = { nullptr };
		BoundPipelineState m_BoundPipeline;
		BoundMeshState m_BoundMesh;

		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;

		RenderTargetState m_RenderTargetState;

		std::vector<Ref<const ComputeShader>> m_UsedComputeShader;
		std::vector<Ref<const Pipeline>> m_UsedPipelines;
	};
}
