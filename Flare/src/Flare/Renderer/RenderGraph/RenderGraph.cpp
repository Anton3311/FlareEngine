#include "RenderGraph.h"

#include "Flare/Renderer/Renderer.h"
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

	void RenderGraph::Execute(Ref<CommandBuffer> commandBuffer)
	{
		for (const auto& node : m_Nodes)
		{
			RenderGraphContext context(Renderer::GetCurrentViewport(), node.RenderTarget);
			node.Pass->OnRender(context, commandBuffer);
		}
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
	}

	void RenderGraph::Clear()
	{
		m_Nodes.clear();
	}
}
