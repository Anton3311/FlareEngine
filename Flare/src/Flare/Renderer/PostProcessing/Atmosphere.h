#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/PostProcessing/PostProcessingEffect.h"


namespace Flare
{
	class AtmospherePass;
	class CommandBuffer;
	class Material;

	struct FLARE_API AtmosphericScatteringParameters
	{
		bool operator==(const AtmosphericScatteringParameters& other) const;
		bool operator!=(const AtmosphericScatteringParameters& other) const;

		float PlanetRadius = 6360.0f;
		float AtmosphereThickness = 100.0f;
		float MieHeight = 1.2f;
		float RayleighHeight = 8.0f;

		glm::vec3 RayleighCoefficients = glm::vec3(5.802f, 13.550f, 33.100f);
		float RayleighAbsorbtion = 0.0f;

		float MieCoefficient = 0.399f;
		float MieAbsorbtion = 4.400f;

		glm::vec3 OzoneAbsorbtion = glm::vec3(0.605f, 1.880f, 0.850f);
	};

	class FLARE_API Atmosphere : public PostProcessingEffect
	{
	public:
		FLARE_TYPE;

		void RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport) override;
		const SerializableObjectDescriptor& GetSerializationDescriptor() const override;
	public:
		// All units are Km
		float ObserverHeight = 0.200f;
		glm::vec3 GroundColor = glm::vec3(0.010f, 0.096f, 0.106f);

		AtmosphericScatteringParameters ScatteringParameters;

		uint32_t ViewRaySteps = 10;
		uint32_t SunTransmittanceSteps = 10;

		uint32_t SunTransmittanceLUTSteps = 100;
		uint32_t SunTransmittanceLUTSize = 256;
	};

	template<>
	struct TypeSerializer<Atmosphere>
	{
		void OnSerialize(Atmosphere& atmosphere, SerializationStream& stream)
		{
			stream.Serialize("PlaneRadius", SerializationValue(atmosphere.ScatteringParameters.PlanetRadius));
			stream.Serialize("AtmosphereThickness", SerializationValue(atmosphere.ScatteringParameters.AtmosphereThickness));
			stream.Serialize("MieHeight", SerializationValue(atmosphere.ScatteringParameters.MieHeight));
			stream.Serialize("RayleighHeight", SerializationValue(atmosphere.ScatteringParameters.RayleighHeight));
			stream.Serialize("ObserverHeight", SerializationValue(atmosphere.ObserverHeight));
			
			stream.Serialize("RayleighCoefficients", SerializationValue(atmosphere.ScatteringParameters.RayleighCoefficients));
			stream.Serialize("RayleighAbsorbtion", SerializationValue(atmosphere.ScatteringParameters.RayleighAbsorbtion));
			stream.Serialize("MieCoefficient", SerializationValue(atmosphere.ScatteringParameters.MieCoefficient));
			stream.Serialize("MieAbsorbtion", SerializationValue(atmosphere.ScatteringParameters.MieAbsorbtion));
			stream.Serialize("OzoneAbsorbtion", SerializationValue(atmosphere.ScatteringParameters.OzoneAbsorbtion));
			stream.Serialize("GroundColor", SerializationValue(atmosphere.GroundColor, SerializationValueFlags::Color));

			stream.Serialize("ViewRaySteps", SerializationValue(atmosphere.ViewRaySteps));
			stream.Serialize("SunTransmittanceSteps", SerializationValue(atmosphere.SunTransmittanceSteps));
			stream.Serialize("SunTransmittanceLUTSteps", SerializationValue(atmosphere.SunTransmittanceLUTSteps));
		}
	};

	class FLARE_API AtmospherePass : public RenderGraphPass
	{
	public:
		AtmospherePass();

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;

		inline Ref<FrameBuffer> GetSunTransmittanceLUT() const { return m_SunTransmittanceLUT; }
	private:
		void GenerateSunTransmittanceLUT(Ref<CommandBuffer> commandBuffer);
	private:
		Ref<FrameBuffer> m_SunTransmittanceLUT = nullptr;
		Ref<Material> m_SunTransmittanceMaterial = nullptr;
		Ref<Material> m_AtmosphereMaterial = nullptr;

		AtmosphericScatteringParameters m_PreviousScatteringParameters;
		uint32_t m_PreviousLUTSteps = 0;

		Ref<Atmosphere> m_Parameters;
	};
}
