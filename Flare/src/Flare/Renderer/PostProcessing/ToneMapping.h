#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Material.h"

namespace Flare
{
	class FLARE_API ToneMapping : public RenderPass
	{
	public:
		FLARE_TYPE;

		ToneMapping();

		void OnRender(RenderingContext& context) override;
	public:
		bool Enabled;
	private:
		Ref<Material> m_Material;
	};

	template<>
	struct TypeSerializer<ToneMapping>
	{
		void OnSerialize(ToneMapping& toneMapping, SerializationStream& stream)
		{
			stream.Serialize("Enabled", SerializationValue(toneMapping.Enabled));
		}
	};
}