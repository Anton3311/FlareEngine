#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/RenderGraph/RenderPassNode.h"
#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"

#include <optional>

namespace Flare
{
	class FrameBuffer;
	class FLARE_API RenderGraph
	{
	public:
		using ResourceId = uint64_t;

		void AddPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass);

		void AddExternalResource(const ExternalRenderGraphResource& resource);

		void Execute(Ref<CommandBuffer> commandBuffer);
		void Build();
		void Clear();
	private:
		void ExecuteLayoutTransitions(Ref<CommandBuffer> commandBuffer, LayoutTransitionsRange range);
	private:
		std::vector<RenderPassNode> m_Nodes;
		std::vector<ExternalRenderGraphResource> m_ExternalResources;

		CompiledRenderGraph m_CompiledRenderGraph;
	};
}
