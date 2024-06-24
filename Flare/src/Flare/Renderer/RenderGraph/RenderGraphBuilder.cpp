#include "RenderGraphBuilder.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

#include <vulkan/vulkan.h>

namespace Flare
{
	struct RenderPassAttachmentKey
	{
		bool operator==(const RenderPassAttachmentKey& other) const
		{
			return Format == other.Format
				&& InitialLayout == other.InitialLayout
				&& FinalLayout == other.FinalLayout
				&& HasClearValue == other.HasClearValue;
		}

		bool operator!=(const RenderPassAttachmentKey& key) const { return !operator==(key); }

		TextureFormat Format = TextureFormat::RGBA8;
		ImageLayout InitialLayout = ImageLayout::Undefined;
		ImageLayout FinalLayout = ImageLayout::Undefined;
		bool HasClearValue = false;
	};

	struct VulkanRenderPassKey
	{
		bool operator==(const VulkanRenderPassKey& other) const
		{
			if (Attachments.size() != other.Attachments.size())
				return false;

			for (size_t i = 0; i < Attachments.size(); i++)
			{
				if (Attachments[i] != other.Attachments[i])
					return false;
			}

			return true;
		}

		inline bool operator!=(const VulkanRenderPassKey& other) const { return !operator==(other); }

		std::vector<RenderPassAttachmentKey> Attachments;
	};

	template<typename T>
	constexpr std::underlying_type_t<T> ToUnderlying(T value)
	{
		return (std::underlying_type_t<T>)value;
	}
}

template<>
struct std::hash<Flare::VulkanRenderPassKey>
{
	size_t operator()(const Flare::VulkanRenderPassKey& key) const
	{
		using namespace Flare;
		size_t hash = 0;

		for (const RenderPassAttachmentKey& attachment : key.Attachments)
		{
			CombineHashes(hash, ToUnderlying(attachment.Format));
			CombineHashes(hash, ToUnderlying(attachment.InitialLayout));
			CombineHashes(hash, ToUnderlying(attachment.FinalLayout));
			CombineHashes(hash, attachment.HasClearValue);
		}

		return hash;
	}
};

namespace Flare
{
	RenderGraphBuilder::RenderGraphBuilder(CompiledRenderGraph& result, Span<RenderPassNode> nodes, Span<ExternalRenderGraphResource> externalResources)
		: m_Result(result), m_Nodes(nodes), m_ExternalResources(externalResources)
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

			ResourceState& state = m_States[GetResoureId(resource.TextureHandle)];
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
			AddTransition(resource.TextureHandle, resource.FinalLayout, m_Result.ExternalResourceFinalTransitions);
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

			FLARE_CORE_ASSERT(output.AttachmentTexture != nullptr);
			ResourceId id = GetResoureId(output.AttachmentTexture);
			auto it = m_States.find(id);

			// Only outputs with AttachmentOutput image layout can be used in a RenderPass
			if (output.Layout == ImageLayout::AttachmentOutput)
			{
				LayoutTransition& transition = m_RenderPassTransitions[nodeIndex][outputIndex];
				transition.TextureHandle = output.AttachmentTexture;
				transition.InitialLayout = GetCurrentLayout(output.AttachmentTexture);
				transition.FinalLayout = output.Layout;

				ResourceState& state = m_States[id];
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

		std::unordered_map<VulkanRenderPassKey, Ref<VulkanRenderPass>> renderPassesCache;

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
					TextureFormat format = outputs[outputIndex].AttachmentTexture->GetFormat();

					RenderPassAttachmentKey& attachmentKey = renderPassKey.Attachments.emplace_back();
					attachmentKey.Format = format;
					attachmentKey.InitialLayout = transition.InitialLayout;
					attachmentKey.FinalLayout = transition.FinalLayout;
					attachmentKey.HasClearValue = outputs[outputIndex].ClearValue.has_value();
				}
			}

			for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
			{
				attachmentTextures.push_back(outputs[outputIndex].AttachmentTexture);
			}

			Ref<VulkanRenderPass> compatibleRenderPass = nullptr;

			auto it = renderPassesCache.find(renderPassKey);
			if (it != renderPassesCache.end())
			{
				compatibleRenderPass = it->second;
			}
			else
			{
				std::optional<uint32_t> depthAttachmentIndex = {};
				for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
				{
					VkAttachmentDescription& description = attachmentDescriptions.emplace_back();
					const LayoutTransition& transition = m_RenderPassTransitions[nodeIndex][outputIndex];
					TextureFormat format = outputs[outputIndex].AttachmentTexture->GetFormat();

					FLARE_CORE_ASSERT(transition.TextureHandle != nullptr);

					bool isDepthAttachment = IsDepthTextureFormat(format);
					if (isDepthAttachment)
					{
						FLARE_CORE_ASSERT(!depthAttachmentIndex);
						depthAttachmentIndex = (uint32_t)outputIndex;
					}

					description.format = TextureFormatToVulkanFormat(format);
					description.flags = 0;
					description.initialLayout = ImageLayoutToVulkanImageLayout(transition.InitialLayout, format);
					description.finalLayout = ImageLayoutToVulkanImageLayout(transition.FinalLayout, format);
					description.samples = VK_SAMPLE_COUNT_1_BIT;
					description.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
					description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
					description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

					if (outputs[outputIndex].ClearValue.has_value())
					{
						description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

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

					if (description.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED && !isDepthAttachment)
					{
						description.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
					}

					FLARE_CORE_ASSERT(description.finalLayout != VK_IMAGE_LAYOUT_UNDEFINED);
				}

				compatibleRenderPass = CreateRef<VulkanRenderPass>(
					Span<VkAttachmentDescription>::FromVector(attachmentDescriptions),
					depthAttachmentIndex);

				if (node.Specifications.HasOutputClearValues())
				{
					compatibleRenderPass->SetDefaultClearValues(Span<VkClearValue>::FromVector(clearValues));
				}

				compatibleRenderPass->SetDebugName(node.Specifications.GetDebugName());

				renderPassesCache.emplace(std::move(renderPassKey), compatibleRenderPass);
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

	void RenderGraphBuilder::AddExplicitTransition(Ref<Texture> texture, ImageLayout layout, LayoutTransitionsRange& transitions)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(texture);
		FLARE_CORE_ASSERT(layout != ImageLayout::Undefined);

		ResourceId id = GetResoureId(texture);
		ResourceStateIterator it = m_States.find(id);

		ImageLayout initialLayout = ImageLayout::Undefined;

		if (it != m_States.end())
		{
			initialLayout = it->second.Layout;
		}

		if (initialLayout == layout)
			return;

		transitions.End++;
		auto& transition = m_Result.LayoutTransitions.emplace_back();
		transition.TextureHandle = texture;
		transition.InitialLayout = initialLayout;
		transition.FinalLayout = layout;

		m_States[id] = { layout, {} };
	}

	void RenderGraphBuilder::AddTransition(Ref<Texture> texture, ImageLayout layout, LayoutTransitionsRange& transitions)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(texture);
		FLARE_CORE_ASSERT(layout != ImageLayout::Undefined);

		ResourceId id = GetResoureId(texture);
		auto it = m_States.find(id);

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

	ImageLayout RenderGraphBuilder::GetCurrentLayout(Ref<Texture> texture)
	{
		FLARE_CORE_ASSERT(texture != nullptr);

		auto it = m_States.find(GetResoureId(texture));
		if (it == m_States.end())
		{
			return ImageLayout::Undefined;
		}

		return it->second.Layout;
	}
}
