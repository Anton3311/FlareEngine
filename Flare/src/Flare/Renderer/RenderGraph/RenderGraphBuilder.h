#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RenderGraph/RenderPassNode.h"

#include <vector>
#include <optional>

namespace Flare
{
	class FLARE_API RenderGraphBuilder
	{
	public:
		using ResourceId = size_t;

		RenderGraphBuilder(Span<RenderPassNode> nodes, Span<LayoutTransition> finalTransitions);

		void Build();
	private:
		void GenerateInputTransitions(size_t nodeIndex);
		void GenerateOutputTransitions(size_t nodeIndex);
		void CreateRenderTargets();
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

		using ResourceStateIterator = std::unordered_map<uint64_t, ResourceState>::const_iterator;

		inline uint64_t GetResoureId(Ref<Texture> texture) const { return (ResourceId)texture.get(); }

		// Adds a layout transition to [transitions].
		// Initial layout is defined by ResourceState.Layout, ImageLayout::Undefined is used instead,
		// in case the state for the provided resource is not present.
		void AddExplicitTransition(Ref<Texture> texture, ImageLayout layout, std::vector<LayoutTransition>& transitions);

		// Adds a layout transition to [transitions].
		// Avoids generating an explicit layout transition, in case an implicit tranition at the end of a render pass can be used.
		void AddTransition(Ref<Texture> texture, ImageLayout layout, std::vector<LayoutTransition>& transitions);

		// Returns ResourceState.Layout for the given texture resource,
		// or ImageLayout::Undefiend, in case the state is not present.
		ImageLayout GetCurrentLayout(Ref<Texture> texture);
	private:
		Span<RenderPassNode> m_Nodes;
		Span<LayoutTransition> m_FinalTransitions;

		std::unordered_map<ResourceId, ResourceState> m_States;

		std::vector<std::vector<LayoutTransition>> m_RenderPassTransitions;
	};
}
