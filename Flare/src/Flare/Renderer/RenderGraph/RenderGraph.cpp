#include "RenderGraph.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderGraph/RenderGraphBuilder.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	void RenderGraph::AddPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass)
	{
		FLARE_CORE_ASSERT(pass != nullptr);

		auto& node = m_Nodes.emplace_back();
		node.Pass = pass;
		node.Specifications = specifications;
	}

	void RenderGraph::AddFinalTransition(Ref<Texture> texture, ImageLayout finalLayout)
	{
		FLARE_CORE_ASSERT(texture != nullptr);

		LayoutTransition& transition = m_FinalTransitions.emplace_back();
		transition.TextureHandle = texture;
		transition.InitialLayout = ImageLayout::Undefined;
		transition.FinalLayout = finalLayout;
	}

	void RenderGraph::Execute(Ref<CommandBuffer> commandBuffer)
	{
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		for (const auto& node : m_Nodes)
		{
			RenderGraphContext context(Renderer::GetCurrentViewport(), node.RenderTarget);

			ExecuteLayoutTransitions(commandBuffer, node.Transitions);

			node.Pass->OnRender(context, commandBuffer);
		}

		ExecuteLayoutTransitions(commandBuffer, m_FinalTransitions);
	}

	void RenderGraph::Build()
	{
		RenderGraphBuilder builder(Span<RenderPassNode>::FromVector(m_Nodes), Span<LayoutTransition>::FromVector(m_FinalTransitions));
		builder.Build();
	}

	void RenderGraph::Clear()
	{
		m_Nodes.clear();
	}

	void RenderGraph::ExecuteLayoutTransitions(Ref<CommandBuffer> commandBuffer, const std::vector<LayoutTransition>& transitions)
	{
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		for (const LayoutTransition& transition : transitions)
		{
			VkImage image = As<VulkanTexture>(transition.TextureHandle)->GetImageHandle();

			TextureFormat format = transition.TextureHandle->GetFormat();
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
}
