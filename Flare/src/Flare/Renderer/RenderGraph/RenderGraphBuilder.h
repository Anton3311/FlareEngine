#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RenderGraph/RenderPassNode.h"
#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"

#include <vector>
#include <optional>

namespace Flare
{
	class FrameBuffer;
	class RenderGraphResourceManager;
	class DependecyGraph;
	class FLARE_API RenderGraphBuilder
	{
	public:
		RenderGraphBuilder(CompiledRenderGraph& result,
			const DependecyGraph& dependecyGraph,
			Span<const RenderPassNode> nodes,
			const RenderGraphResourceManager& resourceManager,
			Span<const ExternalRenderGraphResource> externalResources);

		void Build();
		void CreateRenderTargets(size_t nodeIndex, Ref<FrameBuffer>* outTargets);
		LayoutTransitionsRange GetExplicitTransitions(size_t nodeIndex) const;
	private:
		void GenerateInputTransitions(size_t nodeIndex);
		void GenerateOutputTransitions(size_t nodeIndex);
	private:
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

		using ResourceStateIterator = std::unordered_map<RenderGraphTextureId, ResourceState>::const_iterator;

		// Adds a layout transition to [transitions].
		// Initial layout is defined by ResourceState.Layout, ImageLayout::Undefined is used instead,
		// in case the state for the provided resource is not present.
		void AddExplicitTransition(RenderGraphTextureId texture, ImageLayout layout, LayoutTransitionsRange& transitions);

		// Adds a layout transition to [transitions].
		// Avoids generating an explicit layout transition, in case an implicit tranition at the end of a render pass can be used.
		void AddTransition(RenderGraphTextureId texture, ImageLayout layout, LayoutTransitionsRange& transitions);

		// Returns ResourceState.Layout for the given texture resource,
		// or ImageLayout::Undefiend, in case the state is not present.
		ImageLayout GetCurrentLayout(RenderGraphTextureId texture);
	private:
		struct PassTransitions
		{
			LayoutTransitionsRange ExplicitTransitions;
			std::vector<LayoutTransition> AttachmentTransitions;
		};

		CompiledRenderGraph& m_Result;
		const DependecyGraph& m_DependecyGraph;
		const RenderGraphResourceManager& m_ResourceManager;

		Span<const RenderPassNode> m_Nodes;
		Span<const ExternalRenderGraphResource> m_ExternalResources;

		std::unordered_map<RenderGraphTextureId, ResourceState> m_States;

		std::vector<PassTransitions> m_RenderPassTransitions;
	};
}
