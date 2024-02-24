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

		AtmospherePass()
			: RenderPass(RenderPassQueue::PostProcessing) {}

		void OnRender(RenderingContext& context) override;
	public:
		bool Enabled = true;
		Ref<Material> AtmosphereMaterial = nullptr;
	};

	template<>
	struct TypeSerializer<AtmospherePass>
	{
		void OnSerialize(AtmospherePass& atmosphere, SerializationStream& stream)
		{
			stream.Serialize("Enabled", SerializationValue(atmosphere.Enabled));
			stream.Serialize("AtmosphereMaterial", SerializationValue(atmosphere.AtmosphereMaterial));
		}
	};
}
