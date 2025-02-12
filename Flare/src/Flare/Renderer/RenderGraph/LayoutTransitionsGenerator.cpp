#include "PCH.h"

#include "LayoutTransitionsGenerator.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/RenderGraph/DependecyGraph.h"

namespace Flare
{
	//
	// ResourceState
	//

	ResourceState::ResourceState(uint32_t arrayLayerCount, uint32_t mipCount, ImageLayout initialLayout)
		: m_Dimensions(arrayLayerCount, mipCount)
	{
		m_States = new SubresourceState[m_Dimensions.x * m_Dimensions.y];

		uint32_t stateCount = m_Dimensions.x * m_Dimensions.y;

		for (uint32_t i = 0; i < stateCount; i++)
		{
			m_States->Layout = initialLayout;
		}
	}

	ResourceState::~ResourceState()
	{
		delete[] m_States;
	}

	ImageLayout ResourceState::GetSubresourceRangeLayout(const TextureSubresource& range) const
	{
		FLARE_PROFILE_FUNCTION();

		TextureSubresource validRange = ValidateSubresourceRange(range);
		ImageLayout imageLayout = GetElementAt(validRange.BaseArrayLayer, validRange.BaseMip).Layout;

		FLARE_CORE_ASSERT(IsSameLayout(validRange));

		return imageLayout;
	}

	void ResourceState::SetLayoutAndResetWritingPasses(const TextureSubresource& subresource, ImageLayout newLayout)
	{
		FLARE_PROFILE_FUNCTION();

		TextureSubresource validRange = ValidateSubresourceRange(subresource);

		for (uint32_t y = validRange.BaseMip; y < validRange.BaseMip + validRange.MipCount; y++)
		{
			for (uint32_t x = validRange.BaseArrayLayer; x < validRange.BaseArrayLayer + validRange.ArrayLayerCount; x++)
			{
				SubresourceState& state = GetElementAt(x, y);
				state.LastWritingRenderPass = WritingRenderPass{};
				state.Layout = newLayout;
			}
		}
	}

	void ResourceState::SetWrite(uint32_t renderPassIndex, uint32_t attachmentIndex, const TextureSubresource& subresource, ImageLayout newLayout)
	{
		FLARE_PROFILE_FUNCTION();

		TextureSubresource validRange = ValidateSubresourceRange(subresource);

		for (uint32_t y = validRange.BaseMip; y < validRange.BaseMip + validRange.MipCount; y++)
		{
			for (uint32_t x = validRange.BaseArrayLayer; x < validRange.BaseArrayLayer + validRange.ArrayLayerCount; x++)
			{
				SubresourceState& state = GetElementAt(x, y);
				state.LastWritingRenderPass = WritingRenderPass{};
				state.LastWritingRenderPass.RenderPassIndex = renderPassIndex;
				state.LastWritingRenderPass.AttachmentIndex = attachmentIndex;
				state.Layout = newLayout;
			}
		}
	}

	bool ResourceState::IsSameLayout(const TextureSubresource& subresource) const
	{
		FLARE_PROFILE_FUNCTION();
		TextureSubresource validRange = ValidateSubresourceRange(subresource);
		ImageLayout imageLayout = GetElementAt(validRange.BaseArrayLayer, validRange.BaseMip).Layout;

		for (uint32_t y = validRange.BaseMip; y < validRange.BaseMip + validRange.MipCount; y++)
		{
			for (uint32_t x = validRange.BaseArrayLayer; x < validRange.BaseArrayLayer + validRange.ArrayLayerCount; x++)
			{
				const SubresourceState& state = GetElementAt(x, y);
				if (state.Layout != imageLayout)
					return false;
			}
		}

		return true;
	}

	bool ResourceState::IsSameWritingPass(const TextureSubresource& subresource) const
	{
		FLARE_PROFILE_FUNCTION();
		TextureSubresource validRange = ValidateSubresourceRange(subresource);
		WritingRenderPass renderPass = GetElementAt(validRange.BaseArrayLayer, validRange.BaseMip).LastWritingRenderPass;

		for (uint32_t y = validRange.BaseMip; y < validRange.BaseMip + validRange.MipCount; y++)
		{
			for (uint32_t x = validRange.BaseArrayLayer; x < validRange.BaseArrayLayer + validRange.ArrayLayerCount; x++)
			{
				const SubresourceState& state = GetElementAt(x, y);
				if (state.LastWritingRenderPass != renderPass)
					return false;
			}
		}

		return true;
	}

	bool ResourceState::HasWritingPasses(const TextureSubresource& subresource) const
	{
		FLARE_PROFILE_FUNCTION();

		TextureSubresource validRange = ValidateSubresourceRange(subresource);

		for (uint32_t y = validRange.BaseMip; y < validRange.BaseMip + validRange.MipCount; y++)
		{
			for (uint32_t x = validRange.BaseArrayLayer; x < validRange.BaseArrayLayer + validRange.ArrayLayerCount; x++)
			{
				const SubresourceState& state = GetElementAt(x, y);
				if (state.LastWritingRenderPass.IsValid())
					return true;
			}
		}

		return false;
	}

	std::unordered_set<ResourceState::WritingRenderPass, ResourceState::WritingRenderPassHasher> ResourceState::CollectWritingRenderPasses(
		const TextureSubresource& subresource) const
	{
		FLARE_PROFILE_FUNCTION();

		std::unordered_set<WritingRenderPass, WritingRenderPassHasher> passes;

		TextureSubresource validRange = ValidateSubresourceRange(subresource);
		for (uint32_t y = validRange.BaseMip; y < validRange.BaseMip + validRange.MipCount; y++)
		{
			for (uint32_t x = validRange.BaseArrayLayer; x < validRange.BaseArrayLayer + validRange.ArrayLayerCount; x++)
			{
				const SubresourceState& state = GetElementAt(x, y);

				if (state.LastWritingRenderPass.IsValid())
					passes.emplace(state.LastWritingRenderPass);
			}
		}

		return passes;
	}



	bool ContainsSubresource(const TextureSubresource& subresourceA, const TextureSubresource& subresourceB)
	{
		bool containsMipRange = subresourceB.BaseMip >= subresourceA.BaseMip
			&& (subresourceB.BaseMip + subresourceB.MipCount) <= (subresourceA.BaseMip + subresourceA.MipCount);

		bool containsArrayRange = subresourceB.BaseArrayLayer >= subresourceA.BaseArrayLayer
			&& (subresourceB.BaseArrayLayer + subresourceB.ArrayLayerCount) <= (subresourceA.BaseArrayLayer + subresourceA.ArrayLayerCount);

		return containsMipRange && containsArrayRange;
	}

	//
	// LayoutTransitionsGenerator
	//

	LayoutTransitionsGenerator::LayoutTransitionsGenerator(CompiledRenderGraph& result,
		const DependencyGraph& dependencyGraph,
		Span<const RenderPassNode> nodes,
		const RenderGraphResourceManager& resourceManager,
		Span<const ExternalRenderGraphResource> externalResources)
		: m_Result(result),
		m_DependencyGraph(dependencyGraph),
		m_Nodes(nodes),
		m_ExternalResources(externalResources),
		m_ResourceManager(resourceManager)
	{
	}

	void LayoutTransitionsGenerator::Build()
	{
		FLARE_PROFILE_FUNCTION();

		// Setup initial state for external resources
		for (const auto& resource : m_ExternalResources)
		{
			if (resource.InitialLayout == ImageLayout::Undefined)
				continue;

			const TextureSpecifications& specifications = m_ResourceManager.GetTexture(resource.Texture)->GetSpecifications();

			m_States.try_emplace(resource.Texture, specifications.ArrayLayerCount, specifications.MipCount, resource.InitialLayout);
		}

		m_RenderPassTransitions.resize(m_Nodes.GetSize());

		for (size_t nodeIndex : m_DependencyGraph.GetExecutionOrder())
		{
			m_RenderPassTransitions[nodeIndex].ExplicitTransitions = LayoutTransitionsRange((uint32_t)m_Result.LayoutTransitions.size());

			const RenderPassNode& node = m_Nodes[nodeIndex];
			RenderGraphPassType passType = node.Specifications.GetType();

			if (passType == RenderGraphPassType::Graphics || passType == RenderGraphPassType::Other)
			{
				FLARE_CORE_ASSERT(node.Specifications.GetGeneralTextureResources().size() == 0);

				m_RenderPassTransitions[nodeIndex].AttachmentTransitions.resize(node.Specifications.GetOutputs().size());

				GenerateInputTransitions(nodeIndex);
				GenerateOutputTransitions(nodeIndex);
			}
			else if (passType == RenderGraphPassType::Compute)
			{
				FLARE_CORE_ASSERT(node.Specifications.GetOutputs().size() == 0);

				GenerateInputTransitions(nodeIndex);
				GenerateGeneralResourceTransitions(nodeIndex);
			}
		}

		m_Result.ExternalResourceFinalTransitions = LayoutTransitionsRange((uint32_t)m_Result.LayoutTransitions.size());
		for (const auto& resource : m_ExternalResources)
		{
			AddTransition(resource.Texture,
				TextureSubresource::FULL_VIEW,
				resource.FinalLayout,
				m_Result.ExternalResourceFinalTransitions);
		}
	}

	void LayoutTransitionsGenerator::GenerateInputTransitions(size_t nodeIndex)
	{
		FLARE_PROFILE_FUNCTION();
		const RenderPassNode& node = m_Nodes[nodeIndex];

		for (const auto& input : node.Specifications.GetInputs())
		{
			AddTransition(input.InputTexture,
				TextureSubresource::FULL_VIEW,
				input.Layout,
				m_RenderPassTransitions[nodeIndex].ExplicitTransitions);
		}
	}

	void LayoutTransitionsGenerator::GenerateGeneralResourceTransitions(size_t nodeIndex)
	{
		FLARE_PROFILE_FUNCTION();
		const RenderPassNode& node = m_Nodes[nodeIndex];

		for (const auto& resource : node.Specifications.GetGeneralTextureResources())
		{
			AddTransition(resource.TextureId,
				TextureSubresource::FULL_VIEW,
				ImageLayout::General,
				m_RenderPassTransitions[nodeIndex].ExplicitTransitions);
		}
	}

	void LayoutTransitionsGenerator::GenerateOutputTransitions(size_t nodeIndex)
	{
		FLARE_PROFILE_FUNCTION();
		const RenderPassNode& node = m_Nodes[nodeIndex];
		const auto& outputs = node.Specifications.GetOutputs();

		bool isGraphicsPass = node.Specifications.GetType() == RenderGraphPassType::Graphics;

		for (size_t outputIndex = 0; outputIndex < node.Specifications.GetOutputs().size(); outputIndex++)
		{
			const auto& output = outputs[outputIndex];

			if (isGraphicsPass)
			{
				FLARE_CORE_ASSERT(output.Layout == ImageLayout::AttachmentOutput);

				LayoutTransition& transition = m_RenderPassTransitions[nodeIndex].AttachmentTransitions[outputIndex];
				transition.Texture = output.AttachmentTexture;
				transition.InitialLayout = GetCurrentLayout(output.AttachmentTexture, output.Subresource);
				transition.FinalLayout = output.Layout;

				auto stateIterator = m_States.find(output.AttachmentTexture);
				ResourceState* state = nullptr;

				if (stateIterator != m_States.end())
				{
					state = &stateIterator->second;
				}
				else
				{
					const TextureSpecifications& specifications = m_ResourceManager.GetTexture(output.AttachmentTexture)->GetSpecifications();
					auto insertedEntry = m_States.try_emplace(output.AttachmentTexture, specifications, ImageLayout::Undefined);
					state = &insertedEntry.first->second;
				}

				state->SetWrite((uint32_t)nodeIndex, (uint32_t)outputIndex, output.Subresource, output.Layout);
			}
			else
			{
				AddExplicitTransition(output.AttachmentTexture,
					output.Subresource,
					output.Layout,
					m_RenderPassTransitions[nodeIndex].ExplicitTransitions);
			}
		}
	}

	void LayoutTransitionsGenerator::AddExplicitTransition(RenderGraphTextureId texture,
		const TextureSubresource& subresource,
		ImageLayout layout,
		LayoutTransitionsRange& transitions)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(layout != ImageLayout::Undefined);
		auto it = m_States.find(texture);

		ImageLayout initialLayout = ImageLayout::Undefined;

		ResourceState* resourceState = nullptr;
		if (it == m_States.end())
		{
			// To this point there are were no layout transitions with including this texture, so there is no corresponding `ResourceState`.
			const TextureSpecifications& specifications = m_ResourceManager.GetTexture(texture)->GetSpecifications();
			resourceState = &m_States.try_emplace(texture, specifications, layout).first->second;
		}
		else
		{
			initialLayout = it->second.GetSubresourceRangeLayout(subresource);
			if (initialLayout == layout)
				return;

			resourceState = &it->second;
		}

		transitions.End++;
		auto& transition = m_Result.LayoutTransitions.emplace_back();
		transition.Texture = texture;
		transition.Subresource = subresource;
		transition.InitialLayout = initialLayout;
		transition.FinalLayout = layout;

		resourceState->SetLayoutAndResetWritingPasses(subresource, layout);
	}

	void LayoutTransitionsGenerator::AddTransition(RenderGraphTextureId texture,
		const TextureSubresource& subresource,
		ImageLayout layout,
		LayoutTransitionsRange& transitions)
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

			if (state.HasWritingPasses(subresource))
			{
				auto writingRenderPasses = state.CollectWritingRenderPasses(subresource);
				FLARE_CORE_ASSERT(writingRenderPasses.size() > 0);

				for (ResourceState::WritingRenderPass renderPass : writingRenderPasses)
				{
					const auto& node = m_Nodes[renderPass.RenderPassIndex];
					const auto& output = node.Specifications.GetOutputs()[renderPass.AttachmentIndex];

					// TODO: Handle the case when a "partial render pass transitions" are required.
					// 
					// [Pass A Writes to 0-4 mips] [Pass B Writes to 0-2 mips]
					// [Pass C Reads fro 0-4 mips]
					//
					// In such case:
					// 1. 0-2 mips are synchronized through Pass B
					// 2. 2-4 mips need to be synced using an explicit layout transition (cannot use implicit render pass transitions)
					FLARE_CORE_ASSERT(state.IsSameWritingPass(output.Subresource));

					m_RenderPassTransitions[renderPass.RenderPassIndex].AttachmentTransitions[renderPass.AttachmentIndex].FinalLayout = layout;
				}

				state.SetLayoutAndResetWritingPasses(subresource, layout);
			}
			else
			{
				explicitTransition = true;
			}
		}

		if (explicitTransition)
		{
			AddExplicitTransition(texture, subresource, layout, transitions);
		}
	}

	ImageLayout LayoutTransitionsGenerator::GetCurrentLayout(RenderGraphTextureId texture, const TextureSubresource& subresource)
	{
		FLARE_PROFILE_FUNCTION();
		auto it = m_States.find(texture);
		if (it == m_States.end())
		{
			return ImageLayout::Undefined;
		}

		return it->second.GetSubresourceRangeLayout(subresource);
	}
}
