#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/PostProcessing/PostProcessingEffect.h"


namespace Flare
{
	class AtmospherePass;
	class CommandBuffer;
	class FLARE_API Atmosphere : public PostProcessingEffect
	{
	public:
		FLARE_TYPE;

		void RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport) override;
		const SerializableObjectDescriptor& GetSerializationDescriptor() const override;
	public:
		Ref<Material> AtmosphereMaterial = nullptr;

		// All units are Km
		float PlanetRadius = 6360.0f;
		float AtmosphereThickness = 100.0f;
		float MieHeight = 1.2f;
		float RayleighHeight = 8.0f;

		float ObserverHeight = 0.200f;

		uint32_t ViewRaySteps = 10;
		uint32_t SunTransmittanceSteps = 10;

		uint32_t SunTransmittanceLUTSteps = 100;
		uint32_t SunTransmittanceLUTSize = 512;
	};

	template<>
	struct TypeSerializer<Atmosphere>
	{
		void OnSerialize(Atmosphere& atmosphere, SerializationStream& stream)
		{
			stream.Serialize("AtmosphereMaterial", SerializationValue(atmosphere.AtmosphereMaterial));
			stream.Serialize("PlaneRadius", SerializationValue(atmosphere.PlanetRadius));
			stream.Serialize("AtmosphereThickness", SerializationValue(atmosphere.AtmosphereThickness));
			stream.Serialize("MieHeight", SerializationValue(atmosphere.MieHeight));
			stream.Serialize("RayleighHeight", SerializationValue(atmosphere.RayleighHeight));
			stream.Serialize("ObserverHeight", SerializationValue(atmosphere.ObserverHeight));
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
		void RegenerateSunTransmittanceLUT();

		inline Ref<FrameBuffer> GetSunTransmittanceLUT() const { return m_SunTransmittanceLUT; }
	private:
		void GenerateSunTransmittanceLUT(Ref<CommandBuffer> commandBuffer);
	private:
		bool m_SunTransmittanceLUTIsDirty = true;
		Ref<FrameBuffer> m_SunTransmittanceLUT = nullptr;
		Ref<Material> m_SunTransmittanceMaterial = nullptr;

		Ref<Atmosphere> m_Parameters;
	};
}
