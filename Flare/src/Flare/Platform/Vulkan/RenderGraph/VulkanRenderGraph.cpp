#include "VulkanRenderGraph.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Renderer/RenderGraph/LayoutTransitionsGenerator.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"
#include "Flare/Platform/Vulkan/VulkanRenderPassCache.h"

namespace Flare
{
	VulkanRenderGraph::VulkanRenderGraph(const Viewport& viewport)
		: RenderGraph(viewport)
	{
	}

	void VulkanRenderGraph::Execute(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(IsValid());

		uint32_t frameInFlight = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		const auto& nodes = GetNodes();
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			const RenderPassNode& node = nodes[nodeIndex];

			Ref<FrameBuffer> renderTarget = nullptr;
			if (node.Specifications.GetType() == RenderGraphPassType::Graphics
				&& m_NodeData[nodeIndex].RenderTargetHandleIndex != NodeData::INVALID_TARGET_INDEX)
			{
				renderTarget = m_RenderTargets[m_NodeData[nodeIndex].RenderTargetHandleIndex + frameInFlight];
			}

			RenderGraphContext context(GetViewport(), *this, GetResourceManager(), sceneSubmition, view);

			commandBuffer->BeginLabel(node.Specifications.GetDebugColor(), node.Specifications.GetDebugName());

			node.Pass->OnPrepare(context, commandBuffer);

			ExecuteLayoutTransitions(commandBuffer, m_NodeData[nodeIndex].ExplicitTransitions);

			if (renderTarget)
			{
				commandBuffer->BeginRenderTarget(renderTarget);
				node.Pass->OnRender(context, commandBuffer);
				commandBuffer->EndRenderTarget();
			}
			else
			{
				node.Pass->OnRender(context, commandBuffer);
			}

			commandBuffer->EndLabel();
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
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		for (uint32_t i = range.Start; i < range.End; i++)
		{
			const LayoutTransition& transition = m_CompiledRenderGraph.LayoutTransitions[i];

			Ref<Texture> texture = GetResourceManager().GetTexture(transition.Texture);
			VkImage image = As<VulkanTexture>(texture)->GetImageHandle();

			TextureFormat format = texture->GetFormat();
			VkImageLayout initialLayout = ImageLayoutToVulkanImageLayout(transition.InitialLayout, format);
			VkImageLayout finalLayout = ImageLayoutToVulkanImageLayout(transition.FinalLayout, format);

			if (IsDepthTextureFormat(format))
			{
				vulkanCommandBuffer->TransitionDepthImageLayout(image, HasStencilComponent(format), initialLayout, finalLayout);
			}
			else
			{
				vulkanCommandBuffer->TransitionImageLayout(image, initialLayout, finalLayout);
			}
		}
	}

	void VulkanRenderGraph::CreateRenderTargets(uint32_t frameIndex)
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<Ref<Texture>> attachmentTextures;
		const auto& nodes = GetNodes();
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			if (m_NodeData[nodeIndex].RenderTargetHandleIndex == NodeData::INVALID_TARGET_INDEX)
				continue;

			const auto& outputs = nodes[nodeIndex].Specifications.GetOutputs();
			FLARE_CORE_ASSERT(outputs.size() > 0);

			attachmentTextures.clear();
			attachmentTextures.resize(outputs.size(), nullptr);

			for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
			{
				attachmentTextures[outputIndex] = GetResourceManager().GetTextureForFrameInFlight(outputs[outputIndex].AttachmentTexture, frameIndex);
			}

			uint32_t renderTargetIndex = m_NodeData[nodeIndex].RenderTargetHandleIndex + frameIndex;
			Ref<VulkanFrameBuffer> renderTarget = m_RenderTargets[renderTargetIndex];

			FLARE_CORE_ASSERT(m_NodeData[nodeIndex].VulkanRenderPassHandle);

			m_RenderTargets[renderTargetIndex] = CreateRef<VulkanFrameBuffer>(attachmentTextures[0]->GetWidth(),
				attachmentTextures[0]->GetHeight(),
				m_NodeData[nodeIndex].VulkanRenderPassHandle,
				Span<Ref<Texture>>::FromVector(attachmentTextures),
				false);

			if (renderTarget)
			{
				std::string debugName = renderTarget->GetDebugName();
				m_RenderTargets[renderTargetIndex]->SetDebugName(debugName);
			}
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

			if (node.Specifications.HasOutputClearValues())
			{
				for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
				{
					auto clearValue = outputs[outputIndex].ClearValue.value();

					if (clearValue.Type == AttachmentClearValueType::Color)
					{
						clearValues[outputIndex].color.float32[0] = clearValue.Color.x;
						clearValues[outputIndex].color.float32[1] = clearValue.Color.y;
						clearValues[outputIndex].color.float32[2] = clearValue.Color.z;
						clearValues[outputIndex].color.float32[3] = clearValue.Color.w;
					}
					else
					{
						clearValues[outputIndex].depthStencil.depth = clearValue.Depth;
						clearValues[outputIndex].depthStencil.stencil = 0;
					}
				}
			}

			Ref<VulkanRenderPass> compatibleRenderPass = renderPassCache.GetOrCreate(renderPassKey);

			if (node.Specifications.HasOutputClearValues())
			{
				compatibleRenderPass->SetDefaultClearValues(Span<VkClearValue>::FromVector(clearValues));
			}

			m_NodeData[nodeIndex].VulkanRenderPassHandle = compatibleRenderPass;
		}
	}

	void VulkanRenderGraph::OnPrepare()
	{
		FLARE_PROFILE_FUNCTION();
	}

	void VulkanRenderGraph::OnTexturesResize()
	{
		FLARE_PROFILE_FUNCTION();

		CreateRenderTargets(GraphicsContext::GetInstance().GetCurrentFrameInFlight());
	}

	void VulkanRenderGraph::OnClear()
	{
		FLARE_PROFILE_FUNCTION();

		m_RenderTargets.clear();
		m_NodeData.clear();
	}

	void VulkanRenderGraph::OnBuild()
	{
		FLARE_PROFILE_FUNCTION();

		const auto& nodes = GetNodes();
		const auto& externalResources = GetExternalResources();

		m_NodeData.resize(nodes.size(), NodeData{});

		LayoutTransitionsGenerator builder(m_CompiledRenderGraph,
			GetDependecyGraph(),
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

		m_RenderTargets.resize(renderTargetIndex, nullptr);

		for (uint32_t frameIndex = 0; frameIndex < frameInFlightCount; frameIndex++)
		{
			CreateRenderTargets(frameIndex);
		}
	}
}
