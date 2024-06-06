#include "RenderGraph.h"

#include "Flare/Renderer/Renderer.h"

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

			TransitionLayouts(commandBuffer, node.InputTransitions);

			node.Pass->OnRender(context, commandBuffer);
		}

		TransitionLayouts(commandBuffer, m_FinalTransitions);
	}

	void RenderGraph::Build()
	{
		for (size_t i = 0; i < m_Nodes.size(); i++)
		{
			auto& node = m_Nodes[i];

			std::vector<Ref<Texture>> attachmentTextures;
			attachmentTextures.reserve(node.Specifications.GetOutputs().size());

			for (const auto& output : node.Specifications.GetOutputs())
			{
				attachmentTextures.push_back(output.AttachmentTexture);
			}

			node.RenderTarget = FrameBuffer::Create(Span<Ref<Texture>>::FromVector(attachmentTextures));
			node.RenderTarget->SetDebugName(node.Specifications.GetDebugName());
		}

		GenerateLayoutTransitions();
	}

	void RenderGraph::Clear()
	{
		m_Nodes.clear();
	}

	struct WritingRenderPass
	{
		uint32_t RenderPassIndex = UINT32_MAX;
		uint32_t AttachmentIndex = UINT32_MAX;
	};

	struct ResourceState
	{
		ImageLayout Layout = ImageLayout::Undefined;
		std::optional<WritingRenderPass> LastWritingPass;
	};

	void RenderGraph::GenerateLayoutTransitions()
	{
		std::vector<std::vector<LayoutTransition>> renderPassTransitions;
		renderPassTransitions.reserve(m_Nodes.size());

		std::unordered_map<uint64_t, ResourceState> resourceStates;
		using ResourceIterator = std::unordered_map<uint64_t, ResourceState>::iterator;

		auto transitionLayout = [&resourceStates, &renderPassTransitions](Ref<Texture> texture, ImageLayout finalLayout, std::vector<LayoutTransition>& transitions)
			{
				uint64_t key = (uint64_t)texture.get();
				ResourceIterator it = resourceStates.find(key);

				ImageLayout initialLayout = ImageLayout::Undefined;

				if (it != resourceStates.end())
				{
					initialLayout = it->second.Layout;
				}

				if (initialLayout == finalLayout)
					return;

				auto& transition = transitions.emplace_back();
				transition.TextureHandle = texture;
				transition.InitialLayout = initialLayout;
				transition.FinalLayout = finalLayout;

				resourceStates[key] = { finalLayout, WritingRenderPass{} };
			};

		auto getPreviousImageLayout = [&resourceStates](Ref<Texture> texture) -> ImageLayout
			{
				uint64_t key = (uint64_t)texture.get();
				ResourceIterator it = resourceStates.find(key);

				if (it == resourceStates.end())
					return ImageLayout::Undefined;

				return it->second.Layout;
			};

		for (size_t i = 0; i < m_Nodes.size(); i++)
		{
			auto& node = m_Nodes[i];

			renderPassTransitions.emplace_back().reserve(node.Specifications.GetOutputs().size());

			FLARE_CORE_INFO("Name: {}", node.Specifications.GetDebugName());

			FLARE_CORE_INFO("Input:");
			for (const auto& input : node.Specifications.GetInputs())
			{
				FLARE_CORE_INFO("\t{}", (uint64_t)input.InputTexture.get());
			}

			FLARE_CORE_INFO("Outputs:");
			for (const auto& output : node.Specifications.GetOutputs())
			{
				FLARE_CORE_INFO("\t{}", (uint64_t)output.AttachmentTexture.get());
			}



			for (const auto& input : node.Specifications.GetInputs())
			{
				transitionLayout(input.InputTexture, input.Layout, node.InputTransitions);
			}

			for (const auto& output : node.Specifications.GetOutputs())
			{
				transitionLayout(output.AttachmentTexture, output.Layout, node.InputTransitions);
			}
		}

		for (LayoutTransition& transition : m_FinalTransitions)
		{
			transition.InitialLayout = getPreviousImageLayout(transition.TextureHandle);
		}
	}

	void RenderGraph::TransitionLayouts(Ref<CommandBuffer> commandBuffer, const std::vector<LayoutTransition>& transitions)
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
