#include "RenderGraph.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderGraph/RenderGraphBuilder.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"

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

		RenderPassNode passNode{};
		passNode.Pass = pass;
		passNode.Specifications = specifications;
		m_Nodes.insert(m_Nodes.begin() + index, passNode);
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

	void RenderGraph::Execute(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_IsValid);

		uint32_t frameInFlight = GraphicsContext::GetInstance().GetCurrentFrameInFlight();
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		for (const auto& node : m_Nodes)
		{
			Ref<FrameBuffer> renderTarget = node.RenderTargetHandleIndex == RenderPassNode::INVALID_TARGET_INDEX
				? nullptr
				: m_RenderPassTargets[node.RenderTargetHandleIndex + frameInFlight];

			RenderGraphContext context(
				m_Viewport,
				renderTarget,
				*this, m_ResourceManager,
				sceneSubmition, view);

			commandBuffer->BeginLabel(node.Specifications.GetDebugColor(), node.Specifications.GetDebugName());

			ExecuteLayoutTransitions(commandBuffer, node.Transitions);

			node.Pass->OnRender(context, commandBuffer);

			commandBuffer->EndLabel();
		}

		ExecuteLayoutTransitions(commandBuffer, m_CompiledRenderGraph.ExternalResourceFinalTransitions);
	}

	void RenderGraph::Build()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(!m_IsValid);

		RenderGraphBuilder builder(m_CompiledRenderGraph,
			Span<RenderPassNode>::FromVector(m_Nodes),
			m_ResourceManager,
			Span<ExternalRenderGraphResource>::FromVector(m_ExternalResources),
			m_RenderPassTargets);

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
		m_RenderPassTargets.clear();

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

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();

		std::vector<Ref<Texture>> attachmentTextures;
		for (RenderPassNode& node : m_Nodes)
		{
			if (node.RenderTargetHandleIndex == RenderPassNode::INVALID_TARGET_INDEX)
				continue;

			attachmentTextures.clear();

			for (const auto& output : node.Specifications.GetOutputs())
			{
				attachmentTextures.push_back(m_ResourceManager.GetTexture(output.AttachmentTexture));
			}

			for (uint32_t frameIndex = 0; frameIndex < frameInFlightCount; frameIndex++)
			{
				uint32_t renderTargetIndex = node.RenderTargetHandleIndex + frameIndex;

				Ref<FrameBuffer> renderTarget = m_RenderPassTargets[renderTargetIndex];
				Ref<VulkanRenderPass> compatibleRenderPass = As<VulkanFrameBuffer>(renderTarget)->GetCompatibleRenderPass();

				std::string debugName = renderTarget->GetDebugName();

				m_RenderPassTargets[renderTargetIndex] = CreateRef<VulkanFrameBuffer>(attachmentTextures[0]->GetWidth(),
					attachmentTextures[0]->GetHeight(),
					compatibleRenderPass,
					Span<Ref<Texture>>::FromVector(attachmentTextures),
					false);

				m_RenderPassTargets[renderTargetIndex]->SetDebugName(debugName);
			}
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
