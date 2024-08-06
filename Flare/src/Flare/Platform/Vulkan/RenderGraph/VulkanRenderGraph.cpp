#include "VulkanRenderGraph.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Renderer/RenderGraph/RenderGraphBuilder.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanTexture.h"

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

			RenderGraphContext context(
				GetViewport(),
				renderTarget,
				*this, GetResourceManager(),
				sceneSubmition, view);

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

	void VulkanRenderGraph::CreateRenderTargets()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(IsValid());

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		uint32_t frameIndex = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		std::vector<Ref<Texture>> attachmentTextures;
		const auto& nodes = GetNodes();
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			if (m_NodeData[nodeIndex].RenderTargetHandleIndex == NodeData::INVALID_TARGET_INDEX)
				continue;

			const auto& outputs = nodes[nodeIndex].Specifications.GetOutputs();

			attachmentTextures.clear();
			attachmentTextures.resize(outputs.size(), nullptr);

			for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
			{
				attachmentTextures[outputIndex] = GetResourceManager().GetTextureForFrameInFlight(outputs[outputIndex].AttachmentTexture, frameIndex);
			}

			uint32_t renderTargetIndex = m_NodeData[nodeIndex].RenderTargetHandleIndex + frameIndex;

			Ref<VulkanFrameBuffer> renderTarget = m_RenderTargets[renderTargetIndex];
			Ref<VulkanRenderPass> compatibleRenderPass = renderTarget->GetCompatibleRenderPass();

			std::string debugName = renderTarget->GetDebugName();

			m_RenderTargets[renderTargetIndex] = CreateRef<VulkanFrameBuffer>(attachmentTextures[0]->GetWidth(),
				attachmentTextures[0]->GetHeight(),
				compatibleRenderPass,
				Span<Ref<Texture>>::FromVector(attachmentTextures),
				false);

			m_RenderTargets[renderTargetIndex]->SetDebugName(debugName);
		}
	}

	void VulkanRenderGraph::OnPrepare()
	{
		FLARE_PROFILE_FUNCTION();
	}

	void VulkanRenderGraph::OnTexturesResize()
	{
		FLARE_PROFILE_FUNCTION();

		CreateRenderTargets();
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

		RenderGraphBuilder builder(m_CompiledRenderGraph,
			GetDependecyGraph(),
			Span<const RenderPassNode>(nodes.data(), nodes.size()),
			GetResourceManager(),
			Span<const ExternalRenderGraphResource>(externalResources.data(), externalResources.size()));

		builder.Build();

		std::vector<Ref<FrameBuffer>> temp(GraphicsContext::GetInstance().GetFrameInFlightCount(), nullptr);
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++)
		{
			builder.CreateRenderTargets(nodeIndex, temp.data());

			m_NodeData[nodeIndex].RenderTargetHandleIndex = (uint32_t)m_RenderTargets.size();
			m_NodeData[nodeIndex].ExplicitTransitions = builder.GetExplicitTransitions(nodeIndex);

			if (temp[0] == nullptr)
			{
				m_NodeData[nodeIndex].RenderTargetHandleIndex = NodeData::INVALID_TARGET_INDEX;
				continue;
			}

			for (Ref<FrameBuffer>& target : temp)
			{
				m_RenderTargets.push_back(As<VulkanFrameBuffer>(target));
				target = nullptr;
			}
		}
	}
}
