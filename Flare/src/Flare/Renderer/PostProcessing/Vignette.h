#pragma once

#include "FlareCore/Serialization/Serialization.h"
#include "FlareCore/Serialization/SerializationStream.h"

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/Material.h"

namespace Flare
{
	class FLARE_API Vignette : public RenderPass
	{
	public:
		FLARE_TYPE;

		Vignette();

		void OnRender(RenderingContext& context) override;
	public:
		bool Enabled;
		glm::vec4 Color;
		float Radius;
		float Smoothness;
	private:
		Ref<Material> m_Material;
	};

	template<>
	struct TypeSerializer<Vignette>
	{
		void OnSerialize(Vignette& vignette, SerializationStream& stream)
		{
			stream.Serialize("Enabled", SerializationValue(vignette.Enabled));
			stream.Serialize("Color", SerializationValue(vignette.Color));
			stream.Serialize("Radius", SerializationValue(vignette.Radius));
			stream.Serialize("Smoothness", SerializationValue(vignette.Smoothness));
		}
	};
}