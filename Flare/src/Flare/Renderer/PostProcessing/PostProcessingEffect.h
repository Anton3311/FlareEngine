#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	class RenderGraph;
	class Viewport;
	class SerializableObjectDescriptor;
	class FLARE_API PostProcessingEffect
	{
	public:
		virtual ~PostProcessingEffect() = default;

		virtual void RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport) = 0;
		virtual const SerializableObjectDescriptor& GetSerializationDescriptor() const = 0;
	public:
		inline bool IsEnabled() const { return m_IsEnabled; }
	private:
		bool m_IsEnabled = false;
	};
}
