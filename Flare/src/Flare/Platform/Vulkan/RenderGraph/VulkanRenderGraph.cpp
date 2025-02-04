#include "PCH.h"

#include "VulkanRenderGraph.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Renderer/RenderGraph/LayoutTransitionsGenerator.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanRenderPassCache.h"

namespace Flare
{
	VulkanRenderTarget::VulkanRenderTarget(glm::uvec2 size, Span<const VkImageView> attachments, Ref<VulkanRenderPass> compatibleRenderPass, const char* debugName)
		: m_Size(size), m_CompatibleRenderPass(compatibleRenderPass), m_DebugName(debugName)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(attachments.GetSize() > 0);
		FLARE_CORE_ASSERT(size.x > 0 && size.y > 0);
		FLARE_CORE_ASSERT(compatibleRenderPass);

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.attachmentCount = (uint32_t)attachments.GetSize();
		createInfo.pAttachments = attachments.GetData();
		createInfo.width = size.x;
		createInfo.height = size.y;
		createInfo.renderPass = compatibleRenderPass->GetHandle();
		createInfo.layers = 1;
		createInfo.flags = 0;

		VK_CHECK_RESULT(vkCreateFramebuffer(VulkanContext::GetInstance().GetDevice(), &createInfo, nullptr, &m_Handle));

		VulkanContext::GetInstance().SetDebugName(VK_OBJECT_TYPE_FRAMEBUFFER, (uint64_t)m_Handle, debugName);
	}

	VulkanRenderTarget::VulkanRenderTarget(const VulkanRenderTarget& other)
	{
		FLARE_CORE_ASSERT(!other.IsCreated());
	}

	VulkanRenderTarget::VulkanRenderTarget(VulkanRenderTarget&& other) noexcept
		: m_Size(other.m_Size), m_CompatibleRenderPass(std::move(other.m_CompatibleRenderPass)), m_Handle(other.m_Handle)
	{
		other.m_Size = glm::uvec2(0, 0);
		other.m_Handle = VK_NULL_HANDLE;
	}

	VulkanRenderTarget::~VulkanRenderTarget()
	{
		Release();
	}

	VulkanRenderTarget& VulkanRenderTarget::operator=(VulkanRenderTarget&& other) noexcept
	{
		Release();

		m_Size = other.m_Size;
		m_Handle = other.m_Handle;
		m_DebugName = std::move(other.m_DebugName);

		m_CompatibleRenderPass = std::move(other.m_CompatibleRenderPass);

		other.m_Size = glm::uvec2(0, 0);
		other.m_Handle = VK_NULL_HANDLE;

		return *this;
	}

	VulkanRenderTarget& VulkanRenderTarget::operator=(const VulkanRenderTarget& other)
	{
		FLARE_CORE_ASSERT(!IsCreated() && !other.IsCreated());
		return *this;
	}

	void VulkanRenderTarget::Release()
	{
		FLARE_PROFILE_FUNCTION();

		if (m_Handle)
		{
			vkDestroyFramebuffer(VulkanContext::GetInstance().GetDevice(), m_Handle, nullptr);
			m_Handle = VK_NULL_HANDLE;
		}
	}

	//
	// VulkanRenderGraph
	//

	VulkanRenderGraph::VulkanRenderGraph(World& renderWorld, Entity viewportEntity)
		: RenderGraph(renderWorld, viewportEntity), m_TextureViews(GetResourceManager())
	{
	}

	void VulkanRenderGraph::ExecuteRenderPasses(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(IsValid());

		VulkanCommandBuffer& vulkanCommandBuffer = commandBuffer.DerefAs<VulkanCommandBuffer>();

		uint32_t frameInFlight = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		const auto& nodes = GetNodes();
		const auto& executionOrder = GetDependencyGraph().GetExecutionOrder();
		for (size_t nodeIndex : executionOrder)
		{
			const RenderPassNode& node = nodes[nodeIndex];
			FLARE_CORE_ASSERT(node.Enabled);

			VulkanRenderTarget* renderTarget = nullptr;
			if (node.Specifications.GetType() == RenderGraphPassType::Graphics
				&& m_NodeData[nodeIndex].RenderTargetHandleIndex != NodeData::INVALID_TARGET_INDEX)
			{
				renderTarget = &m_RenderTargets[m_NodeData[nodeIndex].RenderTargetHandleIndex + frameInFlight];
			}

			RenderGraphContext context(
				m_ViewportEntity,
				m_RenderWorld,
				renderTarget ? renderTarget->GetSize() : glm::uvec2(0, 0), // TODO: Specify a valid size even if the render target is null
				*this,
				GetResourceManager(),
				sceneSubmition,
				view);

			vulkanCommandBuffer.BeginLabel(node.Specifications.GetDebugColor(), node.Specifications.GetDebugName());

			node.Pass->OnPrepare(context, commandBuffer);

			ExecuteLayoutTransitions(commandBuffer, m_NodeData[nodeIndex].ExplicitTransitions);

			if (renderTarget)
			{
				ClearValuesRange clearValuesRange = m_NodeData[nodeIndex].ClearValues;

				vulkanCommandBuffer.BeginRenderPass(renderTarget->GetHandle(),
					renderTarget->GetCompatibleRenderPass(),
					renderTarget->GetSize(),
					Span<VkClearValue>::FromVector(m_ClearValuesBuffer).Slice(clearValuesRange.Start, clearValuesRange.Count));

				node.Pass->OnRender(context, commandBuffer);
				vulkanCommandBuffer.EndRenderPass();
			}
			else
			{
				node.Pass->OnRender(context, commandBuffer);
			}

			vulkanCommandBuffer.EndLabel();
		}

		ExecuteLayoutTransitions(commandBuffer, m_CompiledRenderGraph.ExternalResourceFinalTransitions);
	}

	size_t VulkanRenderGraph::GetRenderTargetIndex(size_t nodeIndex) const
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(nodeIndex < m_NodeData.size());

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		size_t renderTargetIndex = m_NodeData[nodeIndex].RenderTargetHandleIndex + (size_t)frameIndex;
		FLARE_CORE_ASSERT(renderTargetIndex < m_RenderTargets.size());
		return renderTargetIndex;
	}

	void VulkanRenderGraph::ExecuteLayoutTransitions(Ref<CommandBuffer> commandBuffer, LayoutTransitionsRange range)
	{
		FLARE_PROFILE_FUNCTION();
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = commandBuffer.As<VulkanCommandBuffer>();

		for (uint32_t i = range.Start; i < range.End; i++)
		{
			const LayoutTransition& transition = m_CompiledRenderGraph.LayoutTransitions[i];

			Ref<Texture> texture = GetResourceManager().GetTexture(transition.Texture);
			VkImage image = texture.As<VulkanTexture>()->GetImageHandle();

			TextureFormat format = texture->GetFormat();
			VkImageLayout initialLayout = ImageLayoutToVulkanImageLayout(transition.InitialLayout, format);
			VkImageLayout finalLayout = ImageLayoutToVulkanImageLayout(transition.FinalLayout, format);

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.image = image;
			barrier.oldLayout = initialLayout;
			barrier.newLayout = finalLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			if (transition.Subresource == TextureSubresource::FULL_VIEW)
			{
				const TextureSpecifications& specifications = texture->GetSpecifications();

				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = specifications.ArrayLayerCount;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = specifications.MipCount;
			}
			else
			{
				barrier.subresourceRange.baseArrayLayer = transition.Subresource.BaseArrayLayer;
				barrier.subresourceRange.layerCount = transition.Subresource.ArrayLayerCount;
				barrier.subresourceRange.baseMipLevel = transition.Subresource.BaseMip;
				barrier.subresourceRange.levelCount = transition.Subresource.MipCount;
			}

			if (IsDepthTextureFormat(format))
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			else
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}

			VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_NONE;
			VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;

			VulkanContext::FillPipelineStagesAndAccessMasks(initialLayout,
				finalLayout,
				sourceStage,
				destinationStage,
				barrier.srcAccessMask,
				barrier.dstAccessMask);

			vkCmdPipelineBarrier(vulkanCommandBuffer->GetHandle(),
				sourceStage, destinationStage, 0, 0,
				nullptr, 0,
				nullptr, 1,
				&barrier);
		}
	}

	void VulkanRenderGraph::CreateRenderTargets(uint32_t frameIndex)
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<VkImageView> attachmentTextures;
		const auto& nodes = GetNodes();

		// TODO: Create render passes only for nodes that are in dependency graph
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			if (m_NodeData[nodeIndex].RenderTargetHandleIndex == NodeData::INVALID_TARGET_INDEX)
				continue;

			CreateRenderTargetForNode(nodeIndex, frameIndex, attachmentTextures);
		}
	}

	void VulkanRenderGraph::SelectVulkanRenderPasses(const LayoutTransitionsGenerator& renderGraphBuilder)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t framesInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		std::vector<Ref<Texture>> attachmentTextures;
		std::vector<VkClearValue> clearValues;

		VulkanRenderPassCache& renderPassCache = VulkanContext::GetInstance().GetRenderPassCache();

		const auto& nodes = GetNodes();
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			const RenderPassNode& node = nodes[nodeIndex];
			if (!node.Enabled)
				continue;

			// RenderTargets are only created for Graphics render passes
			if (node.Specifications.GetType() != RenderGraphPassType::Graphics)
				continue;

			attachmentTextures.clear();

			const auto& outputs = nodes[nodeIndex].Specifications.GetOutputs();

			if (node.Specifications.HasOutputClearValues())
			{
				clearValues.clear();
				clearValues.resize(outputs.size());
			}

			if (outputs.size() == 0)
				continue;

			VulkanRenderPassKey renderPassKey;

			{
				FLARE_PROFILE_SCOPE("GenerateRenderPassKey");

				renderPassKey.Attachments.reserve(outputs.size());

				const auto& attachmentTransitions = renderGraphBuilder.GetRenderPassAttachmentTransitions(nodeIndex);
				for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
				{
					const LayoutTransition& transition = attachmentTransitions[outputIndex];
					TextureFormat format = GetResourceManager().GetTextureFormat(outputs[outputIndex].AttachmentTexture);

					RenderPassAttachmentKey& attachmentKey = renderPassKey.Attachments.emplace_back();
					attachmentKey.Format = format;
					attachmentKey.InitialLayout = transition.InitialLayout;
					attachmentKey.FinalLayout = transition.FinalLayout;
					attachmentKey.HasClearValue = outputs[outputIndex].ClearValue.has_value();
				}
			}

			m_NodeData[nodeIndex].VulkanRenderPassHandle = renderPassCache.GetOrCreate(renderPassKey);
		}
	}

	static VkClearValue AttachmentClearValueToVkClearValue(AttachmentClearValue clearValue)
	{
		VkClearValue result{};
		switch (clearValue.Type)
		{
		case AttachmentClearValueType::Color:
			result.color.float32[0] = clearValue.Color.r;
			result.color.float32[1] = clearValue.Color.g;
			result.color.float32[2] = clearValue.Color.b;
			result.color.float32[3] = clearValue.Color.a;
			break;
		case AttachmentClearValueType::Depth:
			result.depthStencil.depth = clearValue.Depth;
			result.depthStencil.stencil = 0;
			break;
		default:
			FLARE_VERIFY_UNREACHABLE();
		}

		return result;
	}

	void VulkanRenderGraph::FillClearValuesBuffer()
	{
		FLARE_PROFILE_FUNCTION();

		size_t bufferSize = 0;

		for (const RenderPassNode& node : GetNodes())
		{
			if (node.Specifications.HasOutputClearValues())
				bufferSize += node.Specifications.GetOutputs().size();
		}

		m_ClearValuesBuffer.resize(bufferSize);

		const auto& nodes = GetNodes();

		size_t allocationOffset = 0;
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			const RenderGraphPassSpecifications& specifications = nodes[nodeIndex].Specifications;
			if (!specifications.HasOutputClearValues())
				continue;

			const ClearValuesRange range = ClearValuesRange{ (uint32_t)allocationOffset, (uint32_t)specifications.GetOutputs().size() };
			m_NodeData[nodeIndex].ClearValues = range;
			allocationOffset += specifications.GetOutputs().size();

			for (uint32_t i = 0; i < range.Count; i++)
			{
				const auto& outputClearValue = specifications.GetOutputs()[i].ClearValue;

				if (outputClearValue.has_value())
					m_ClearValuesBuffer[range.Start + i] = AttachmentClearValueToVkClearValue(*outputClearValue);
				else
					m_ClearValuesBuffer[range.Start + i] = VkClearValue{};
			}
		}
	}

	void VulkanRenderGraph::CreateRenderTargetForNode(size_t nodeIndex, uint32_t frameIndex, std::vector<VkImageView>& temporaryAttachmentsStorage)
	{
		FLARE_PROFILE_FUNCTION();

		const auto& nodes = GetNodes();
		const auto& outputs = nodes[nodeIndex].Specifications.GetOutputs();
		FLARE_CORE_ASSERT(outputs.size() > 0);

		temporaryAttachmentsStorage.clear();
		temporaryAttachmentsStorage.resize(outputs.size(), nullptr);

		glm::uvec2 renderTargetSize = glm::uvec2(0, 0);

		for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
		{
			const auto& output = outputs[outputIndex];

			Span<const VkImageView> imageViews = m_TextureViews.GetOrCreate(output.AttachmentTexture, output.Subresource);
			temporaryAttachmentsStorage[outputIndex] = imageViews[frameIndex];

			Ref<const Texture> texture = GetResourceManager().GetTexture(output.AttachmentTexture);
			renderTargetSize.x = glm::max(renderTargetSize.x, texture->GetWidth());
			renderTargetSize.y = glm::max(renderTargetSize.y, texture->GetHeight());
		}

		uint32_t renderTargetIndex = m_NodeData[nodeIndex].RenderTargetHandleIndex + frameIndex;
		VulkanRenderTarget& renderTarget = m_RenderTargets[renderTargetIndex];

		FLARE_CORE_ASSERT(m_NodeData[nodeIndex].VulkanRenderPassHandle);

		std::string renderTargetDebugName = fmt::format("{}.#{}", nodes[nodeIndex].Specifications.GetDebugName(), frameIndex);

		renderTarget = std::move(VulkanRenderTarget(renderTargetSize,
			Span<const VkImageView>::FromVector(temporaryAttachmentsStorage),
			m_NodeData[nodeIndex].VulkanRenderPassHandle,
			renderTargetDebugName.c_str()));
	}

	void VulkanRenderGraph::OnPrepare()
	{
		FLARE_PROFILE_FUNCTION();
	}

	void VulkanRenderGraph::OnTexturesResize()
	{
		FLARE_PROFILE_FUNCTION();

		OnClear();
		OnBuild();

		// TODO: Resize the textures
#if 0
		m_TextureViews.Clear();
		CreateRenderTargets(GraphicsContext::GetInstance().GetCurrentFrameInFlight());
#endif
	}

	void VulkanRenderGraph::OnClear()
	{
		FLARE_PROFILE_FUNCTION();

		m_RenderTargets.clear();
		m_NodeData.clear();

		VkDevice device = VulkanContext::GetInstance().GetDevice();
		m_TextureViews.Clear();
	}

	void VulkanRenderGraph::OnBuild()
	{
		FLARE_PROFILE_FUNCTION();

		const auto& nodes = GetNodes();
		const auto& externalResources = GetExternalResources();

		m_NodeData.resize(nodes.size(), NodeData{});

		LayoutTransitionsGenerator builder(m_CompiledRenderGraph,
			GetDependencyGraph(),
			Span<const RenderPassNode>(nodes.data(), nodes.size()),
			GetResourceManager(),
			Span<const ExternalRenderGraphResource>(externalResources.data(), externalResources.size()));

		builder.Build();

		SelectVulkanRenderPasses(builder);

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		uint32_t renderTargetIndex = 0;
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			const auto& spec = nodes[nodeIndex].Specifications;
			m_NodeData[nodeIndex].ExplicitTransitions = builder.GetExplicitTransitions(nodeIndex);

			if (spec.GetType() == RenderGraphPassType::Graphics && spec.GetOutputs().size() > 0)
			{
				m_NodeData[nodeIndex].RenderTargetHandleIndex = renderTargetIndex;
				renderTargetIndex += frameInFlightCount;
			}
		}

		m_RenderTargets.resize(renderTargetIndex);

		for (uint32_t frameIndex = 0; frameIndex < frameInFlightCount; frameIndex++)
		{
			CreateRenderTargets(frameIndex);
		}

		FillClearValuesBuffer();
	}

	void VulkanRenderGraph::OnAfterAllocatingOnDemandTextures(const std::unordered_set<RenderGraphTextureId>& updatedTextures)
	{
		FLARE_PROFILE_FUNCTION();

		for (RenderGraphTextureId texture : updatedTextures)
		{
			m_TextureViews.RecreateCachedTextureViews(texture);
		}

		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		std::vector<VkImageView> temporaryAttachmentsStorage;

		const auto& dependencyGraph = GetDependencyGraph();
		for (size_t nodeIndex : dependencyGraph.GetExecutionOrder())
		{
			const auto& node = GetNodes()[nodeIndex];

			if (m_NodeData[nodeIndex].RenderTargetHandleIndex == NodeData::INVALID_TARGET_INDEX)
				continue;

			bool recreateRenderPass = false;
			for (const auto& output : node.Specifications.GetOutputs())
			{
				if (updatedTextures.contains(output.AttachmentTexture))
				{
					recreateRenderPass = true;
					break;
				}
			}

			if (recreateRenderPass)
			{
				CreateRenderTargetForNode(nodeIndex, frameIndex, temporaryAttachmentsStorage);
			}
		}
	}
}
