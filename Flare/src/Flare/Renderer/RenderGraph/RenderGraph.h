#pragma once

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class FrameBuffer;
	class VulkanRenderPass;
	class FLARE_API RenderGraph
	{
	public:
		struct LayoutTransition
		{
			Ref<Texture> TextureHandle = nullptr;
			ImageLayout InitialLayout = ImageLayout::Undefined;
			ImageLayout FinalLayout = ImageLayout::Undefined;
		};

		using ResourceId = uint64_t;

		void AddPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass);

		// Adds a layout transitions for a given texture.
		// The transition is performed after finishing all the render passes in the graph.
		// Initial layout is infered based on the inputs & outputs of render passes in the graph.
		void AddFinalTransition(Ref<Texture> texture, ImageLayout finalLayout);

		void Execute(Ref<CommandBuffer> commandBuffer);
		void Build();
		void Clear();

		void GenerateLayoutTransitions();
	private:
		void TransitionLayouts(Ref<CommandBuffer> commandBuffer, const std::vector<LayoutTransition>& transitions);
	private:
		struct RenderPassNode
		{
			RenderGraphPassSpecifications Specifications;
			Ref<RenderGraphPass> Pass = nullptr;
			Ref<FrameBuffer> RenderTarget = nullptr;
			std::vector<LayoutTransition> InputTransitions;
		};

		std::vector<RenderPassNode> m_Nodes;
		std::vector<LayoutTransition> m_FinalTransitions;
	};
}
