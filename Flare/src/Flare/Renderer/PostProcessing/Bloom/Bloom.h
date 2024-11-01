#pragma once

#include "Flare/Renderer/PostProcessing/PostProcessingEffect.h"

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore/Serialization/SerializationStream.h"
#include "FlareCore/Serialization/TypeInitializer.h"
#include "FlareCore/Serialization/Metadata.h"

namespace Flare
{
	class Bloom : public PostProcessingEffect
	{
	public:
		FLARE_TYPE;
		FLARE_SERIALIZABLE;

		void RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport) override;
		const SerializableObjectDescriptor& GetSerializationDescriptor() const override;
	public:
		float Threshold = 1.0f;
		float Intensity = 1.0f;
	};

	template<>
	struct TypeSerializer<Bloom>
	{
		void OnSerialize(Bloom& bloom, SerializationStream& stream)
		{
			stream.Serialize("Threshold", SerializationValue(bloom.Threshold));
			stream.Serialize("Intensity", SerializationValue(bloom.Intensity));
		}
	};
}
