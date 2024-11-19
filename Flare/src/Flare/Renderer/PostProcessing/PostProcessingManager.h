#pragma once

#include "FlareECS/Entity/Entity.h"

#include "Flare/Renderer/PostProcessing/PostProcessingEffect.h"

#include <optional>

namespace Flare
{
	class RenderGraph;
	class World;
	class FLARE_API PostProcessingManager
	{
	public:
		struct PostProcessingEntry
		{
			const SerializableObjectDescriptor* Descriptor = nullptr;
			Ref<PostProcessingEffect> Effect = nullptr;
		};

		void AddEffect(Ref<PostProcessingEffect> effect);
		void RegisterRenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld);

		void RegisterRenderPasses(RenderGraph& renderGraph,
			Entity viewportEntity,
			const World& renderWorld,
			PostProcessingExecutionOrder group);

		std::optional<Ref<PostProcessingEffect>> FindEffect(const SerializableObjectDescriptor& descriptor) const;

		template<typename T>
		std::optional<Ref<T>> GetEffect()
		{
			std::optional<Ref<PostProcessingEffect>> effect = FindEffect(FLARE_SERIALIZATION_DESCRIPTOR_OF(T));
			if (effect)
				return effect->As<T>();

			return {};
		}

		void MarkAsDirty();

		// TODO: This should exist
		inline void ResetDirtyFlag() { m_IsDirty = false; }

		inline const std::vector<PostProcessingEntry>& GetEntries() const { return m_Entries; }
		inline bool IsDirty() const { return m_IsDirty; }
	private:
		std::vector<PostProcessingEntry> m_Entries;
		bool m_Initialized = false;
		bool m_IsDirty = false;
	};
}