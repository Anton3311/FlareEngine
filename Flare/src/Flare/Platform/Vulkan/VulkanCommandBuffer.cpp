#include "VulkanCommandBuffer.h"

#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"
#include "Flare/Platform/Vulkan/VulkanIndexBuffer.h"
#include "Flare/Platform/Vulkan/VulkanMaterial.h"
#include "Flare/Platform/Vulkan/VulkanGPUTimer.h"

namespace Flare
{
	VulkanCommandBuffer::VulkanCommandBuffer(VkCommandBuffer commandBuffer)
		: m_CommandBuffer(commandBuffer) {}

	void VulkanCommandBuffer::Reset()
	{
		VK_CHECK_RESULT(vkResetCommandBuffer(m_CommandBuffer, 0));
	}

	void VulkanCommandBuffer::Begin()
	{
		VkCommandBufferBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		VK_CHECK_RESULT(vkBeginCommandBuffer(m_CommandBuffer, &info));
	}

	void VulkanCommandBuffer::End()
	{
		VK_CHECK_RESULT(vkEndCommandBuffer(m_CommandBuffer));
		m_UsedMaterials.clear();
	}

	void VulkanCommandBuffer::BeginRenderPass(const Ref<VulkanRenderPass>& renderPass, const Ref<VulkanFrameBuffer>& frameBuffer)
	{
		VkRenderPassBeginInfo info{};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.framebuffer = frameBuffer->GetHandle();
		info.renderPass = renderPass->GetHandle();
		info.renderArea.offset = { 0, 0 };
		info.renderArea.extent.width = frameBuffer->GetSize().x;
		info.renderArea.extent.height = frameBuffer->GetSize().y;
		info.pClearValues = nullptr;

		const FrameBufferSpecifications& specifications = frameBuffer->GetSpecifications();

		vkCmdBeginRenderPass(m_CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);

		m_CurrentRenderPass = renderPass;
	}

	void VulkanCommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(m_CommandBuffer);
		m_CurrentRenderPass = nullptr;
	}

	static void GetPipelineStagesAndAccessFlags(VkImageLayout oldLayout, VkImageLayout newLayout,
		VkAccessFlags& srcAccess, VkAccessFlags& dstAccess,
		VkPipelineStageFlags& srcStage, VkPipelineStageFlags& dstStage)
	{
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			srcAccess = VK_ACCESS_NONE;
			dstAccess= VK_ACCESS_TRANSFER_WRITE_BIT;

			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccess = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // NOTE: Add vertex stage if image is sampled in vertex shader?
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			srcAccess = VK_ACCESS_TRANSFER_READ_BIT;
			dstAccess = VK_ACCESS_SHADER_READ_BIT;

			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
	}

	void VulkanCommandBuffer::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkPipelineStageFlags sourceStage = 0;
		VkPipelineStageFlags destinationStage = 0;

		GetPipelineStagesAndAccessFlags(oldLayout, newLayout, barrier.srcAccessMask, barrier.dstAccessMask, sourceStage, destinationStage);

		vkCmdPipelineBarrier(m_CommandBuffer,
			sourceStage, destinationStage, 0, 0,
			nullptr, 0,
			nullptr, 1,
			&barrier);
	}

	void VulkanCommandBuffer::TransitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t baseMipLevel, uint32_t mipLevels)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseMipLevel = baseMipLevel;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkPipelineStageFlags sourceStage = 0;
		VkPipelineStageFlags destinationStage = 0;

		GetPipelineStagesAndAccessFlags(oldLayout, newLayout, barrier.srcAccessMask, barrier.dstAccessMask, sourceStage, destinationStage);

		vkCmdPipelineBarrier(m_CommandBuffer,
			sourceStage, destinationStage, 0, 0,
			nullptr, 0,
			nullptr, 1,
			&barrier);
	}

	void VulkanCommandBuffer::TransitionDepthImageLayout(VkImage image, bool hasStencilComponent, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent)
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		VkPipelineStageFlags sourceStage = 0;
		VkPipelineStageFlags destinationStage = 0;

		if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_NONE;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_NONE;
			barrier.dstAccessMask = VK_ACCESS_NONE;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		}

		vkCmdPipelineBarrier(m_CommandBuffer,
			sourceStage, destinationStage, 0, 0,
			nullptr, 0,
			nullptr, 1,
			&barrier);
	}

	void VulkanCommandBuffer::ClearImage(VkImage image, const glm::vec4& clearColor, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageSubresourceRange range{};
		range.baseArrayLayer = 0;
		range.layerCount = 1;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange = range;

			vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}

		VkClearColorValue value;
		value.float32[0] = clearColor.r;
		value.float32[1] = clearColor.g;
		value.float32[2] = clearColor.b;
		value.float32[3] = clearColor.a;

		vkCmdClearColorImage(m_CommandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &value, 1, &range);

		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange = range;

			vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}
	}

	void VulkanCommandBuffer::ClearDepthStencilImage(VkImage image, bool hasStencilComponent, float depthValue, uint32_t stencilValue, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkImageSubresourceRange range{};
		range.baseArrayLayer = 0;
		range.layerCount = 1;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent)
			range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange = range;

			vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}

		VkClearDepthStencilValue clearValue{};
		clearValue.depth = depthValue;
		clearValue.stencil = stencilValue;
		vkCmdClearDepthStencilImage(m_CommandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearValue, 1, &range);

		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange = range;

			vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}
	}

	void VulkanCommandBuffer::CopyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D size)
	{
		VkBufferImageCopy copyRegion{};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageSubresource.mipLevel = 0;

		copyRegion.imageOffset = { 0, 0, 0 };
		copyRegion.imageExtent = size;

		vkCmdCopyBufferToImage(m_CommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
	}

	void VulkanCommandBuffer::CopyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, size_t size, size_t sourceOffset, size_t destinationOffset)
	{
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = sourceOffset;
		copyRegion.dstOffset = destinationOffset;
		copyRegion.size = size;

		vkCmdCopyBuffer(m_CommandBuffer, sourceBuffer, destinationBuffer, 1, &copyRegion);
	}

	void VulkanCommandBuffer::GenerateImageMipMaps(VkImage image, uint32_t mipLevels, glm::uvec2 imageSize)
	{
		for (uint32_t i = 1; i < mipLevels; i++)
		{
			VkImageBlit blit{};
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.layerCount = 1;

			blit.srcOffsets[1].x = (int32_t)(imageSize.x >> (i - 1));
			blit.srcOffsets[1].y = (int32_t)(imageSize.y >> (i - 1));
			blit.srcOffsets[1].z = 1;

			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.layerCount = 1;

			blit.dstOffsets[1].x = (int32_t)(imageSize.x >> i);
			blit.dstOffsets[1].y = (int32_t)(imageSize.y >> i);
			blit.dstOffsets[1].z = 1;

			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;
				barrier.subresourceRange.baseMipLevel = i;
				barrier.subresourceRange.levelCount = 1;

				vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			}

			vkCmdBlitImage(m_CommandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit, VK_FILTER_LINEAR);

			{
				VkImageMemoryBarrier barrier{};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				barrier.image = image;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = 1;
				barrier.subresourceRange.baseMipLevel = i;
				barrier.subresourceRange.levelCount = 1;

				vkCmdPipelineBarrier(m_CommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
			}
		}
	}

	void VulkanCommandBuffer::BindPipeline(const Ref<const Pipeline>& pipeline)
	{
		auto vulkanPipeline = As<const VulkanPipeline>(pipeline);
		vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetHandle());
		m_CurrentPipeline = vulkanPipeline;
	}

	void VulkanCommandBuffer::BindVertexBuffers(const Span<Ref<const VertexBuffer>>& vertexBuffers)
	{
		std::vector<VkDeviceSize> offsets(vertexBuffers.GetSize());
		std::vector<VkBuffer> buffers(vertexBuffers.GetSize());
		for (size_t i = 0; i < vertexBuffers.GetSize(); i++)
		{
			offsets[i] = 0;
			buffers[i] = As<const VulkanVertexBuffer>(vertexBuffers[i])->GetHandle();
		}

		vkCmdBindVertexBuffers(m_CommandBuffer, 0, (uint32_t)vertexBuffers.GetSize(), buffers.data(), offsets.data());
	}

	void VulkanCommandBuffer::BindIndexBuffer(const Ref<const IndexBuffer>& indexBuffer)
	{
		vkCmdBindIndexBuffer(m_CommandBuffer,
			As<const VulkanIndexBuffer>(indexBuffer)->GetHandle(), 0,
			indexBuffer->GetIndexFormat() == IndexBuffer::IndexFormat::UInt16
				? VK_INDEX_TYPE_UINT16
				: VK_INDEX_TYPE_UINT32);
	}

	void VulkanCommandBuffer::BindDescriptorSet(const Ref<const VulkanDescriptorSet>& descriptorSet, VkPipelineLayout pipelineLayout, uint32_t index)
	{
		VkDescriptorSet setHandle = descriptorSet->GetHandle();
		vkCmdBindDescriptorSets(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, index, 1, &setHandle, 0, nullptr);
	}

	void VulkanCommandBuffer::SetPrimaryDescriptorSet(const Ref<VulkanDescriptorSet>& set)
	{
		m_PrimaryDescriptorSet = set;
	}

	void VulkanCommandBuffer::SetSecondaryDescriptorSet(const Ref<VulkanDescriptorSet>& set)
	{
		m_SecondaryDescriptorSet = set;
	}

	void VulkanCommandBuffer::ApplyMaterial(const Ref<const Material>& material)
	{
		FLARE_CORE_ASSERT(material->GetShader());
		Ref<VulkanMaterial> vulkanMaterial = As<VulkanMaterial>(std::const_pointer_cast<Material>(material));
		Ref<Pipeline> pipeline = vulkanMaterial->GetPipeline(m_CurrentRenderPass);
		VkPipelineLayout pipelineLayout = As<const VulkanPipeline>(vulkanMaterial->GetPipeline(m_CurrentRenderPass))->GetLayoutHandle();
		Ref<const ShaderMetadata> metadata = material->GetShader()->GetMetadata();

		BindPipeline(pipeline);

		if (m_PrimaryDescriptorSet)
			BindDescriptorSet(m_PrimaryDescriptorSet, pipelineLayout, 0);
		if (m_SecondaryDescriptorSet)
			BindDescriptorSet(m_SecondaryDescriptorSet, pipelineLayout, 1);

		Ref<VulkanDescriptorSet> materialDescriptorSet = vulkanMaterial->GetDescriptorSet();
		if (materialDescriptorSet)
		{
			vulkanMaterial->UpdateDescriptorSet();
			BindDescriptorSet(materialDescriptorSet, pipelineLayout, 2);
		}

		for (size_t i = 0; i < metadata->PushConstantsRanges.size(); i++)
		{
			const ShaderPushConstantsRange& range = metadata->PushConstantsRanges[i];
			if (range.Size == 0)
				continue;

			VkShaderStageFlags stage = 0;
			switch (range.Stage)
			{
			case ShaderStageType::Vertex:
				stage = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderStageType::Pixel:
				stage = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
			}

			vkCmdPushConstants(m_CommandBuffer, pipelineLayout, stage, (uint32_t)range.Offset, (uint32_t)range.Size, material->GetPropertiesBuffer() + range.Offset);
		}
	}

	void VulkanCommandBuffer::SetViewportAndScisors(Math::Rect viewportRect)
	{
		VkRect2D scissors{};
		scissors.offset.x = (int32_t)viewportRect.Min.x;
		scissors.offset.y = (int32_t)viewportRect.Min.y;
		scissors.extent.width = (int32_t)viewportRect.GetWidth();
		scissors.extent.height = (int32_t)viewportRect.GetHeight();

		VkViewport viewport{};
		viewport.width = viewportRect.GetWidth();
		viewport.height = viewportRect.GetHeight();
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.x = viewportRect.Min.x;
		viewport.y = viewportRect.Min.y;

		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissors);
	}

	void VulkanCommandBuffer::DrawIndexed(uint32_t indicesCount)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indicesCount, 1, 0, 0, 0);
	}

	void VulkanCommandBuffer::DrawIndexed(uint32_t firstIndex, uint32_t indicesCount)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indicesCount, 1, firstIndex, 0, 0);
	}

	void VulkanCommandBuffer::DrawIndexed(uint32_t firstIndex, uint32_t indicesCount, uint32_t firstInstance, uint32_t instancesCount)
	{
		vkCmdDrawIndexed(m_CommandBuffer, indicesCount, instancesCount, firstIndex, 0, firstInstance);
	}

	void VulkanCommandBuffer::DrawIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t firstInstance, uint32_t instancesCount)
	{
		Ref<const VertexBuffer> vertexBuffers[] =
		{
			mesh->GetVertices(),
			mesh->GetNormals(),
			mesh->GetTangents(),
			mesh->GetUVs()
		};

		BindVertexBuffers(Span(vertexBuffers, 4));
		BindIndexBuffer(mesh->GetIndexBuffer());

		const auto& subMesh = mesh->GetSubMeshes()[subMeshIndex];
		vkCmdDrawIndexed(m_CommandBuffer, subMesh.IndicesCount, instancesCount, subMesh.BaseIndex, subMesh.BaseVertex, firstInstance);
	}

	void VulkanCommandBuffer::DepthImagesBarrier(Span<VkImage> images, bool hasStencil,
		VkPipelineStageFlags srcStage, VkAccessFlags srcAccessMask,
		VkPipelineStageFlags dstStage, VkAccessFlags dstAccessMask,
		VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		m_ImageBarriers.clear();
		m_ImageBarriers.reserve(images.GetSize());

		for (size_t i = 0; i < images.GetSize(); i++)
		{
			VkImageMemoryBarrier& barrier = m_ImageBarriers.emplace_back();
			barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = images[i];
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcAccessMask = srcAccessMask;
			barrier.dstAccessMask = dstAccessMask;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			if (hasStencil)
				barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;

			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.subresourceRange.levelCount = 1;
		}

		vkCmdPipelineBarrier(m_CommandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, (uint32_t)images.GetSize(), m_ImageBarriers.data());
	}

	void VulkanCommandBuffer::BeginTimer(Ref<VulkanGPUTimer> timer, VkPipelineStageFlagBits pipelineStages)
	{
		vkCmdResetQueryPool(m_CommandBuffer, timer->GetPoolHandle(), 0, 2);
		vkCmdWriteTimestamp(m_CommandBuffer, pipelineStages, timer->GetPoolHandle(), 0);
	}

	void VulkanCommandBuffer::EndTimer(Ref<VulkanGPUTimer> timer, VkPipelineStageFlagBits pipelineStages)
	{
		vkCmdWriteTimestamp(m_CommandBuffer, pipelineStages, timer->GetPoolHandle(), 1);
	}
}
