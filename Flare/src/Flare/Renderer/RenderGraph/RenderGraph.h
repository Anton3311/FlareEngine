#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class FrameBuffer;
	class VulkanRenderPass;

	class FLARE_API RenderGraph
	{
	public:
		using ResourceId = uint64_t;

		void AddPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass);

		void Execute(Ref<CommandBuffer> commandBuffer);
		void Build();
		void Clear();
	private:
		struct RenderPassNode
		{
			RenderGraphPassSpecifications Specifications;
			Ref<RenderGraphPass> Pass = nullptr;
			Ref<FrameBuffer> RenderTarget = nullptr;
		};

		std::vector<RenderPassNode> m_Nodes;
	};
}
