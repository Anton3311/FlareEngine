#include "RenderGraphBuilder.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	RenderGraphBuilder::RenderGraphBuilder(CompiledRenderGraph& result,
		Span<RenderPassNode> nodes,
		const RenderGraphResourceManager& resourceManager,
		Span<ExternalRenderGraphResource> externalResources)
		: m_Result(result), m_Nodes(nodes), m_ExternalResources(externalResources), m_ResourceManager(resourceManager)
	{
	}

	void RenderGraphBuilder::Build()
	{
		FLARE_PROFILE_FUNCTION();

		// Setup initial state for external resources
		for (const auto& resource : m_ExternalResources)
		{
			if (resource.InitialLayout == ImageLayout::Undefined)
				continue;

			ResourceState& state = m_States[resource.Texture];
			state.Layout = resource.InitialLayout;
		}

		m_RenderPassTransitions.reserve(m_Nodes.GetSize());

		for (size_t nodeIndex = 0; nodeIndex < m_Nodes.GetSize(); nodeIndex++)
		{
			m_Nodes[nodeIndex].Transitions = LayoutTransitionsRange((uint32_t)m_Result.LayoutTransitions.size());
			GenerateInputTransitions(nodeIndex);
			GenerateOutputTransitions(nodeIndex);
		}

		m_Result.ExternalResourceFinalTransitions = LayoutTransitionsRange((uint32_t)m_Result.LayoutTransitions.size());
		for (const auto& resource : m_ExternalResources)
		{
			AddTransition(resource.Texture, resource.FinalLayout, m_Result.ExternalResourceFinalTransitions);
		}

		CreateRenderTargets();
	}

	void RenderGraphBuilder::GenerateInputTransitions(size_t nodeIndex)
	{
		FLARE_PROFILE_FUNCTION();
		RenderPassNode& node = m_Nodes[nodeIndex];

		m_RenderPassTransitions.emplace_back().resize(node.Specifications.GetOutputs().size());

		for (const auto& input : node.Specifications.GetInputs())
		{
			AddTransition(input.InputTexture, input.Layout, node.Transitions);
		}
	}

	void RenderGraphBuilder::GenerateOutputTransitions(size_t nodeIndex)
	{
		FLARE_PROFILE_FUNCTION();
		RenderPassNode& node = m_Nodes[nodeIndex];
		const auto& outputs = node.Specifications.GetOutputs();
		for (size_t outputIndex = 0; outputIndex < node.Specifications.GetOutputs().size(); outputIndex++)
		{
			const auto& output = outputs[outputIndex];
			auto it = m_States.find(output.AttachmentTexture);

			// Only outputs with AttachmentOutput image layout can be used in a RenderPass
			if (output.Layout == ImageLayout::AttachmentOutput)
			{
				LayoutTransition& transition = m_RenderPassTransitions[nodeIndex][outputIndex];
				transition.Texture = output.AttachmentTexture;
				transition.InitialLayout = GetCurrentLayout(output.AttachmentTexture);
				transition.FinalLayout = output.Layout;

				ResourceState& state = m_States[output.AttachmentTexture];
				state.Layout = output.Layout;
				state.LastWritingPass = WritingRenderPass{};
				state.LastWritingPass->RenderPassIndex = (uint32_t)nodeIndex;
				state.LastWritingPass->AttachmentIndex = (uint32_t)outputIndex;
			}
			else
			{
				AddExplicitTransition(output.AttachmentTexture, output.Layout, node.Transitions);
			}
		}
	}

	void RenderGraphBuilder::CreateRenderTargets()
	{
		FLARE_PROFILE_FUNCTION();

		std::vector<VkAttachmentDescription> attachmentDescriptions;
		std::vector<Ref<Texture>> attachmentTextures;
		std::vector<VkClearValue> clearValues;

		VulkanRenderPassCache& renderPassCache = VulkanContext::GetInstance().GetRenderPassCache();

		for (size_t nodeIndex = 0; nodeIndex < m_Nodes.GetSize(); nodeIndex++)
		{
			RenderPassNode& node = m_Nodes[nodeIndex];

			// RenderTargets are only created for Graphics render passes
			if (node.Specifications.GetType() != RenderGraphPassType::Graphics)
				continue;

			attachmentDescriptions.clear();
			attachmentTextures.clear();

			const auto& outputs = m_Nodes[nodeIndex].Specifications.GetOutputs();

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
				for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
				{
					const LayoutTransition& transition = m_RenderPassTransitions[nodeIndex][outputIndex];
					TextureFormat format = m_ResourceManager.GetTextureFormat(outputs[outputIndex].AttachmentTexture);

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

			for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
			{
				attachmentTextures.push_back(m_ResourceManager.GetTexture(outputs[outputIndex].AttachmentTexture));
			}

			Ref<VulkanRenderPass> compatibleRenderPass = renderPassCache.GetOrCreate(renderPassKey);

			if (node.Specifications.HasOutputClearValues())
			{
				compatibleRenderPass->SetDefaultClearValues(Span<VkClearValue>::FromVector(clearValues));
			}

			node.RenderTarget = CreateRef<VulkanFrameBuffer>(
				attachmentTextures[0]->GetWidth(),
				attachmentTextures[0]->GetHeight(),
				compatibleRenderPass,
				Span<Ref<Texture>>::FromVector(attachmentTextures),
				false);

			node.RenderTarget->SetDebugName(node.Specifications.GetDebugName());
		}
	}

	void RenderGraphBuilder::AddExplicitTransition(RenderGraphTextureId texture, ImageLayout layout, LayoutTransitionsRange& transitions)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(layout != ImageLayout::Undefined);
		ResourceStateIterator it = m_States.find(texture);

		ImageLayout initialLayout = ImageLayout::Undefined;

		if (it != m_States.end())
		{
			initialLayout = it->second.Layout;
		}

		if (initialLayout == layout)
			return;

		transitions.End++;
		auto& transition = m_Result.LayoutTransitions.emplace_back();
		transition.Texture = texture;
		transition.InitialLayout = initialLayout;
		transition.FinalLayout = layout;

		m_States[texture] = { layout, {} };
	}

	void RenderGraphBuilder::AddTransition(RenderGraphTextureId texture, ImageLayout layout, LayoutTransitionsRange& transitions)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(layout != ImageLayout::Undefined);

		auto it = m_States.find(texture);

		bool explicitTransition = false;
		if (it == m_States.end())
		{
			explicitTransition = true;
		}
		else
		{
			ResourceState& state = it->second;

			if (state.LastWritingPass)
			{
				auto lastRenderPass = state.LastWritingPass;
				m_RenderPassTransitions[lastRenderPass->RenderPassIndex][lastRenderPass->AttachmentIndex].FinalLayout = layout;

				state.Layout = layout;
				state.LastWritingPass = {};
			}
			else
			{
				explicitTransition = true;
			}
		}

		if (explicitTransition)
		{
			AddExplicitTransition(texture, layout, transitions);
		}
	}

	ImageLayout RenderGraphBuilder::GetCurrentLayout(RenderGraphTextureId texture)
	{
		auto it = m_States.find(texture);
		if (it == m_States.end())
		{
			return ImageLayout::Undefined;
		}

		return it->second.Layout;
	}
}
