#include "PostProcessingManager.h"

namespace Flare
{
	void PostProcessingManager::AddEffect(Ref<PostProcessingEffect> effect)
	{
		auto& entry = m_Entries.emplace_back();
		entry.Effect = effect;
		entry.Descriptor = &effect->GetSerializationDescriptor();
	}

	void PostProcessingManager::RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		for (const auto& entry : m_Entries)
		{
			entry.Effect->RegisterRenderPasses(renderGraph, viewport);
		}
	}

	std::optional<Ref<PostProcessingEffect>> PostProcessingManager::FindEffect(const SerializableObjectDescriptor& descriptor) const
	{
		for (const auto& entry : m_Entries)
		{
			if (entry.Descriptor == &descriptor)
			{
				return entry.Effect;
			}
		}

		return {};
	}
}
