#include "RenderGraphBuilder.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/RenderGraph/DependecyGraph.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	RenderGraphBuilder::RenderGraphBuilder(CompiledRenderGraph& result,
		const DependecyGraph& dependecyGraph,
		Span<const RenderPassNode> nodes,
		const RenderGraphResourceManager& resourceManager,
		Span<const ExternalRenderGraphResource> externalResources)
		: m_Result(result),
		m_DependecyGraph(dependecyGraph),
		m_Nodes(nodes),
		m_ExternalResources(externalResources),
		m_ResourceManager(resourceManager)
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

		m_RenderPassTransitions.resize(m_Nodes.GetSize());

		for (size_t nodeIndex : m_DependecyGraph.GetExecutionOrder())
		{
			m_RenderPassTransitions[nodeIndex].ExplicitTransitions = LayoutTransitionsRange((uint32_t)m_Result.LayoutTransitions.size());
			GenerateInputTransitions(nodeIndex);
			GenerateOutputTransitions(nodeIndex);
		}

		m_Result.ExternalResourceFinalTransitions = LayoutTransitionsRange((uint32_t)m_Result.LayoutTransitions.size());
		for (const auto& resource : m_ExternalResources)
		{
			AddTransition(resource.Texture, resource.FinalLayout, m_Result.ExternalResourceFinalTransitions);
		}
	}

	void RenderGraphBuilder::GenerateInputTransitions(size_t nodeIndex)
	{
		FLARE_PROFILE_FUNCTION();
		const RenderPassNode& node = m_Nodes[nodeIndex];

		m_RenderPassTransitions[nodeIndex].AttachmentTransitions.resize(node.Specifications.GetOutputs().size());

		for (const auto& input : node.Specifications.GetInputs())
		{
			AddTransition(input.InputTexture, input.Layout, m_RenderPassTransitions[nodeIndex].ExplicitTransitions);
		}
	}

	void RenderGraphBuilder::GenerateOutputTransitions(size_t nodeIndex)
	{
		FLARE_PROFILE_FUNCTION();
		const RenderPassNode& node = m_Nodes[nodeIndex];
		const auto& outputs = node.Specifications.GetOutputs();
		for (size_t outputIndex = 0; outputIndex < node.Specifications.GetOutputs().size(); outputIndex++)
		{
			const auto& output = outputs[outputIndex];
			auto it = m_States.find(output.AttachmentTexture);

			// Only outputs with AttachmentOutput image layout can be used in a RenderPass
			if (output.Layout == ImageLayout::AttachmentOutput)
			{
				LayoutTransition& transition = m_RenderPassTransitions[nodeIndex].AttachmentTransitions[outputIndex];
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
				AddExplicitTransition(output.AttachmentTexture, output.Layout, m_RenderPassTransitions[nodeIndex].ExplicitTransitions);
			}
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
				m_RenderPassTransitions[lastRenderPass->RenderPassIndex].AttachmentTransitions[lastRenderPass->AttachmentIndex].FinalLayout = layout;

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
