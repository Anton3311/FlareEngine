#pragma once

#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/RenderGraph/RenderPassNode.h"
#include "Flare/Renderer/RenderGraph/RenderGraphCommon.h"

#include <vector>
#include <optional>

namespace Flare
{
	class FLARE_API ResourceState
	{
	public:
		FLARE_NONCOPYABLE(ResourceState);

		ResourceState(uint32_t arrayLayerCount, uint32_t mipCount, ImageLayout initialLayout);

		inline ResourceState(const TextureSpecifications& specifications, ImageLayout imageLayout)
			: ResourceState(specifications.ArrayLayerCount, specifications.MipCount, imageLayout) {}

		~ResourceState();

		struct WritingRenderPass
		{
			constexpr bool IsValid() const { return RenderPassIndex != UINT32_MAX && AttachmentIndex != UINT32_MAX; }

			constexpr bool operator==(WritingRenderPass other) const
			{
				return other.RenderPassIndex == RenderPassIndex && other.AttachmentIndex == AttachmentIndex;
			}

			constexpr bool operator!=(WritingRenderPass other) const
			{
				return !operator==(other);
			}

			uint32_t RenderPassIndex = UINT32_MAX;
			uint32_t AttachmentIndex = UINT32_MAX;
		};

		struct WritingRenderPassHasher
		{
			inline size_t operator()(const WritingRenderPass& writingPass) const
			{
				size_t hash = 0;
				CombineHashes(hash, writingPass.AttachmentIndex);
				CombineHashes(hash, writingPass.RenderPassIndex);

				return hash;
			}
		};

		struct SubresourceState
		{
			ImageLayout Layout = ImageLayout::Undefined;
			WritingRenderPass LastWritingRenderPass;
		};

		ImageLayout GetSubresourceRangeLayout(const TextureSubresource& range) const;
		void SetLayoutAndResetWritingPasses(const TextureSubresource& subresource, ImageLayout newLayout);
		void SetWrite(uint32_t renderPassIndex, uint32_t attachmentIndex, const TextureSubresource& subresource, ImageLayout newLayout);

		bool IsSameLayout(const TextureSubresource& subresource) const;
		bool IsSameWritingPass(const TextureSubresource& subresource) const;

		bool HasWritingPasses(const TextureSubresource& subresource) const;

		std::unordered_set<WritingRenderPass, WritingRenderPassHasher> CollectWritingRenderPasses(const TextureSubresource& subresource) const;
	private:
		inline SubresourceState& GetElementAt(uint32_t arrayLayer, uint32_t mipLevel)
		{
			// arrayLayer - x
			// mipLevel - y

			FLARE_CORE_ASSERT(arrayLayer < m_Dimensions.x && mipLevel < m_Dimensions.y);
			return m_States[m_Dimensions.x * mipLevel + arrayLayer];
		}

		inline const SubresourceState& GetElementAt(uint32_t arrayLayer, uint32_t mipLevel) const
		{
			// arrayLayer - x
			// mipLevel - y

			FLARE_CORE_ASSERT(arrayLayer < m_Dimensions.x && mipLevel < m_Dimensions.y);
			return m_States[m_Dimensions.x * mipLevel + arrayLayer];
		}

		TextureSubresource ValidateSubresourceRange(const TextureSubresource& range) const
		{
			FLARE_CORE_ASSERT(range.ArrayLayerCount > 0 && range.MipCount > 0);

			if (range == TextureSubresource::FULL_VIEW)
			{
				TextureSubresource full{};
				full.BaseArrayLayer = 0;
				full.ArrayLayerCount = m_Dimensions.x;
				full.BaseMip = 0;
				full.MipCount = m_Dimensions.y;

				return full;
			}

			return range;
		}
	private:
		glm::uvec2 m_Dimensions;
		SubresourceState* m_States;
	};

	// Checks whether subresourceA contains subresourceB
	bool ContainsSubresource(const TextureSubresource& subresourceA, const TextureSubresource& subresourceB);
	
	class RenderGraphResourceManager;
	class DependencyGraph;
	class FLARE_API LayoutTransitionsGenerator
	{
	public:
		FLARE_NONCOPYABLE(LayoutTransitionsGenerator);

		LayoutTransitionsGenerator(CompiledRenderGraph& result,
			const DependencyGraph& dependencyGraph,
			Span<const RenderPassNode> nodes,
			const RenderGraphResourceManager& resourceManager,
			Span<const ExternalRenderGraphResource> externalResources);

		void Build();

		inline LayoutTransitionsRange GetExplicitTransitions(size_t nodeIndex) const
		{
			return m_RenderPassTransitions[nodeIndex].ExplicitTransitions;
		}

	 	inline const std::vector<LayoutTransition>& GetRenderPassAttachmentTransitions(size_t nodeIndex) const
		{
			return m_RenderPassTransitions[nodeIndex].AttachmentTransitions;
		}
	private:
		void GenerateInputTransitions(size_t nodeIndex);
		void GenerateGeneralResourceTransitions(size_t nodeIndex);
		void GenerateOutputTransitions(size_t nodeIndex);
	private:
		using ResourceStateIterator = std::unordered_map<RenderGraphTextureId, ResourceState>::const_iterator;

		// Adds a layout transition to [transitions].
		// Initial layout is defined by ResourceState.Layout, ImageLayout::Undefined is used instead,
		// in case the state for the provided resource is not present.
		void AddExplicitTransition(RenderGraphTextureId texture,
			const TextureSubresource& subresource,
			ImageLayout layout,
			LayoutTransitionsRange& transitions);

		// Adds a layout transition to [transitions].
		// Avoids generating an explicit layout transition, in case an implicit transition at the end of a render pass can be used.
		void AddTransition(RenderGraphTextureId texture,
			const TextureSubresource& subresource,
			ImageLayout layout,
			LayoutTransitionsRange& transitions);

		// Returns ResourceState.Layout for the given texture resource,
		// or ImageLayout::Undefined, in case the state is not present.
		ImageLayout GetCurrentLayout(RenderGraphTextureId texture, const TextureSubresource& subresource);
	private:
		struct PassTransitions
		{
			LayoutTransitionsRange ExplicitTransitions;
			std::vector<LayoutTransition> AttachmentTransitions;
		};

		CompiledRenderGraph& m_Result;
		const DependencyGraph& m_DependencyGraph;
		const RenderGraphResourceManager& m_ResourceManager;

		Span<const RenderPassNode> m_Nodes;
		Span<const ExternalRenderGraphResource> m_ExternalResources;

		std::unordered_map<RenderGraphTextureId, ResourceState> m_States;

		std::vector<PassTransitions> m_RenderPassTransitions;
	};
}
