#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Material.h"

namespace Flare
{
	class FLARE_API AtmospherePass : public RenderPass
	{
	public:
		FLARE_TYPE;

		void OnRender(RenderingContext& context) override;
	public:
		Ref<Material> AtmosphereMaterial = nullptr;
		glm::vec3 SunLightWaveLengths;
	};

	template<>
	struct TypeSerializer<AtmospherePass>
	{
		void OnSerialize(AtmospherePass& atmosphere, SerializationStream& stream)
		{
			stream.Serialize("AtmosphereMaterial", SerializationValue(atmosphere.AtmosphereMaterial));
			stream.Serialize("SunLightWaveLengths", SerializationValue(atmosphere.SunLightWaveLengths));
		}
	};
}
