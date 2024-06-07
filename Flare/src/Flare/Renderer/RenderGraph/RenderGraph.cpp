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

			TransitionLayouts(commandBuffer, node.Transitions);

			node.Pass->OnRender(context, commandBuffer);
		}

		TransitionLayouts(commandBuffer, m_FinalTransitions);
	}

	void RenderGraph::Build()
	{
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

			renderPassTransitions.emplace_back().resize(node.Specifications.GetOutputs().size());

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
				uint64_t key = (uint64_t)input.InputTexture.get();
				auto it = resourceStates.find(key);

				bool explicitTransition = false;
				if (it == resourceStates.end())
				{
					explicitTransition = true;
				}
				else
				{
					ResourceState& state = it->second;

					if (state.LastWritingPass)
					{
						renderPassTransitions[state.LastWritingPass->RenderPassIndex][state.LastWritingPass->AttachmentIndex].FinalLayout = input.Layout;
						state.Layout = input.Layout;
						state.LastWritingPass = {};
					}
					else
					{
						explicitTransition = true;
					}
				}

				if (explicitTransition)
				{
					transitionLayout(input.InputTexture, input.Layout, node.Transitions);
				}
			}

			size_t outputIndex = 0;
			for (const auto& output : node.Specifications.GetOutputs())
			{
				uint64_t key = (uint64_t)output.AttachmentTexture.get();
				auto it = resourceStates.find(key);

				// Only outputs with AttachmentOutput image layout can be used in a RenderPass
				if (output.Layout == ImageLayout::AttachmentOutput)
				{
					uint64_t key = (uint64_t)output.AttachmentTexture.get();

					LayoutTransition& transition = renderPassTransitions[i][outputIndex];
					transition.InitialLayout = getPreviousImageLayout(output.AttachmentTexture);
					transition.FinalLayout = output.Layout;

					ResourceState& state = resourceStates[key];
					state.LastWritingPass = WritingRenderPass{};
					state.LastWritingPass->RenderPassIndex = (uint32_t)i;
					state.LastWritingPass->AttachmentIndex = (uint32_t)outputIndex;
				}
				else
				{
					transitionLayout(output.AttachmentTexture, output.Layout, node.Transitions);
				}

				outputIndex++;
			}
		}

		for (LayoutTransition& transition : m_FinalTransitions)
		{
			transition.InitialLayout = getPreviousImageLayout(transition.TextureHandle);
		}

		std::vector<VkAttachmentDescription> attachmentDescriptions;
		std::vector<Ref<Texture>> attachmentTextures;
		for (size_t nodeIndex = 0; nodeIndex < m_Nodes.size(); nodeIndex++)
		{
			RenderPassNode& node = m_Nodes[nodeIndex];

			// RenderTargets are only created for Graphics render passes
			if (node.Specifications.GetType() != RenderGraphPassType::Graphics)
				continue;

			attachmentDescriptions.clear();
			attachmentTextures.clear();

			std::optional<uint32_t> depthAttachmentIndex = {};

			const auto& outputs = m_Nodes[nodeIndex].Specifications.GetOutputs();
			for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
			{
				VkAttachmentDescription& description = attachmentDescriptions.emplace_back();
				const LayoutTransition& transition = renderPassTransitions[nodeIndex][outputIndex];
				TextureFormat format = outputs[outputIndex].AttachmentTexture->GetFormat();

				if (IsDepthTextureFormat(format))
				{
					FLARE_CORE_ASSERT(!depthAttachmentIndex);
					depthAttachmentIndex = (uint32_t)outputIndex;
				}

				description.format = TextureFormatToVulkanFormat(format);
				description.flags = 0;
				description.initialLayout = ImageLayoutToVulkanImageLayout(transition.InitialLayout, format);
				description.finalLayout = ImageLayoutToVulkanImageLayout(transition.FinalLayout, format);
				description.samples = VK_SAMPLE_COUNT_1_BIT;
				description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

				if (description.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED)
				{
					description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				}

				FLARE_CORE_ASSERT(description.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED);

				attachmentTextures.push_back(outputs[outputIndex].AttachmentTexture);
			}

			Ref<VulkanRenderPass> compatibleRenderPass = CreateRef<VulkanRenderPass>(
				Span<VkAttachmentDescription>::FromVector(attachmentDescriptions),
				depthAttachmentIndex);

			node.RenderTarget = CreateRef<VulkanFrameBuffer>(
				attachmentTextures[0]->GetWidth(),
				attachmentTextures[0]->GetHeight(),
				compatibleRenderPass,
				Span<Ref<Texture>>::FromVector(attachmentTextures),
				true);

			node.RenderTarget->SetDebugName(node.Specifications.GetDebugName());
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
