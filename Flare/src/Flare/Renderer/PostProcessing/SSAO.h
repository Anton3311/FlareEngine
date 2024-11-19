#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/ShaderConstantBuffer.h"
#include "Flare/Renderer/ShaderDescriptorBuffer.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/PostProcessing/PostProcessingEffect.h"

namespace Flare
{
	class ComputeShader;
	class Material;

	class FLARE_API SSAO : public PostProcessingEffect
	{
	public:
		FLARE_TYPE;
		FLARE_SERIALIZABLE;

		SSAO();

		void RegisterRenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld) override;
		const SerializableObjectDescriptor& GetSerializationDescriptor() const override;
	private:
		void RegisterHBAORenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld);
	public:
		float Bias = 30.0f;
		float Radius = 1.0f;
		float Intensity = 1.0f;
		float Sharpness = 1.0f;
	};

	template<>
	struct TypeSerializer<SSAO>
	{
		void OnSerialize(SSAO& ssao, SerializationStream& stream)
		{
			stream.Serialize("Radius", SerializationValue(ssao.Radius));
			stream.Serialize("Bias", SerializationValue(ssao.Bias));
			stream.Serialize("Intensity", SerializationValue(ssao.Intensity));
			stream.Serialize("Sharpness", SerializationValue(ssao.Sharpness));
		}
	};
}
