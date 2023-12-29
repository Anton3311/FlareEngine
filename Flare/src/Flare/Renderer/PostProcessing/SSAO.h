#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Material.h"

namespace Flare
{
	class FLARE_API SSAO : public RenderPass
	{
	public:
		FLARE_SERIALIZABLE;

		SSAO();

		void OnRender(RenderingContext& context) override;
	public:
		float Bias;
		float Radius;
		float BlurSize;
	private:
		Ref<FrameBuffer> m_AOTargets[2];
		Ref<Material> m_Material;
		Ref<Material> m_BlurMaterial;

		std::optional<uint32_t> m_BiasPropertyIndex;
		std::optional<uint32_t> m_RadiusPropertyIndex;

		std::optional<uint32_t> m_BlurSizePropertyIndex;
		std::optional<uint32_t> m_TexelSizePropertyIndex;
	};

	template<>
	struct TypeSerializer<SSAO>
	{
		void OnSerialize(SSAO& ssao, SerializationStream& stream)
		{
			stream.Serialize("Radius", SerializationValue(ssao.Radius));
			stream.Serialize("Bias", SerializationValue(ssao.Bias));
			stream.Serialize("BlurSize", SerializationValue(ssao.BlurSize));
		}
	};
}
