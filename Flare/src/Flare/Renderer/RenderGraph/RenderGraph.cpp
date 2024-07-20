#include "RenderGraph.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderGraph/RenderGraphBuilder.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	RenderGraph::RenderGraph(const Viewport& viewport)
		: m_Viewport(viewport), m_ResourceManager(viewport)
	{
	}

	void RenderGraph::AddPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass)
	{
		FLARE_CORE_ASSERT(pass != nullptr);

		auto& node = m_Nodes.emplace_back();
		node.Pass = pass;
		node.Specifications = specifications;
	}

	void RenderGraph::InsertPass(const RenderGraphPassSpecifications& specifications, Ref<RenderGraphPass> pass, size_t index)
	{
		FLARE_CORE_ASSERT(pass != nullptr);
		FLARE_CORE_ASSERT(index <= m_Nodes.size());

		m_Nodes.insert(m_Nodes.begin() + index, RenderPassNode{ specifications, pass, nullptr, {} });
	}

	const RenderPassNode* RenderGraph::GetRenderPassNode(size_t index) const
	{
		if (index < m_Nodes.size())
			return &m_Nodes[index];

		return nullptr;
	}

	std::optional<size_t> RenderGraph::FindPassByName(std::string_view name) const
	{
		FLARE_PROFILE_FUNCTION();
		for (size_t i = 0; i < m_Nodes.size(); i++)
		{
			if (m_Nodes[i].Specifications.GetDebugName() == name)
			{
				return i;
			}
		}

		return {};
	}

	void RenderGraph::AddExternalResource(const ExternalRenderGraphResource& resource)
	{
		FLARE_CORE_ASSERT(resource.FinalLayout != ImageLayout::Undefined);
		m_ExternalResources.push_back(resource);
	}

	void RenderGraph::Execute(Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_IsValid);

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		for (const auto& node : m_Nodes)
		{
			RenderGraphContext context(Renderer::GetCurrentViewport(), node.RenderTarget, *this, m_ResourceManager);

			ExecuteLayoutTransitions(commandBuffer, node.Transitions);

			node.Pass->OnRender(context, commandBuffer);
		}

		ExecuteLayoutTransitions(commandBuffer, m_CompiledRenderGraph.ExternalResourceFinalTransitions);
	}

	void RenderGraph::Build()
	{
		FLARE_PROFILE_FUNCTION();
		RenderGraphBuilder builder(m_CompiledRenderGraph,
			Span<RenderPassNode>::FromVector(m_Nodes),
			m_ResourceManager,
			Span<ExternalRenderGraphResource>::FromVector(m_ExternalResources));

		builder.Build();

		m_NeedsRebuilding = false;
		m_IsValid = true;
	}

	void RenderGraph::Clear()
	{
		FLARE_PROFILE_FUNCTION();
		m_Nodes.clear();
		m_CompiledRenderGraph.Reset();
		m_ResourceManager.Clear();

		m_IsValid = false;
	}

	void RenderGraph::OnViewportResize()
	{
		FLARE_PROFILE_FUNCTION();

		m_ResourceManager.ResizeTextures();
		CreateRenderTargets();
	}

	void RenderGraph::CreateRenderTargets()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_IsValid);

		std::vector<Ref<Texture>> attachmentTextures;
		for (RenderPassNode& node : m_Nodes)
		{
			if (node.RenderTarget == nullptr)
				continue;

			attachmentTextures.clear();

			for (const auto& output : node.Specifications.GetOutputs())
			{
				attachmentTextures.push_back(m_ResourceManager.GetTexture(output.AttachmentTexture));
			}

			Ref<VulkanRenderPass> compatibleRenderPass = As<VulkanFrameBuffer>(node.RenderTarget)->GetCompatibleRenderPass();

			node.RenderTarget = CreateRef<VulkanFrameBuffer>(attachmentTextures[0]->GetWidth(),
				attachmentTextures[0]->GetHeight(),
				compatibleRenderPass,
				Span<Ref<Texture>>::FromVector(attachmentTextures),
				false);
		}
	}

	void RenderGraph::ExecuteLayoutTransitions(Ref<CommandBuffer> commandBuffer, LayoutTransitionsRange range)
	{
		FLARE_PROFILE_FUNCTION();
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		for (uint32_t i = range.Start; i < range.End; i++)
		{
			const LayoutTransition& transition = m_CompiledRenderGraph.LayoutTransitions[i];

			Ref<Texture> texture = m_ResourceManager.GetTexture(transition.Texture);
			VkImage image = As<VulkanTexture>(texture)->GetImageHandle();

			TextureFormat format = texture->GetFormat();
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
