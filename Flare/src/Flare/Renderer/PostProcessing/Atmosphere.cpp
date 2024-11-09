#include "PCH.h"

#include "Atmosphere.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Sampler.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include <glm/gtc/epsilon.hpp>

namespace Flare
{
	bool AtmosphericScatteringParameters::operator==(const AtmosphericScatteringParameters& other) const
	{
		constexpr float epsilon = glm::epsilon<float>();
		glm::bvec3 rayleighCoefficientsEqual = glm::epsilonEqual(RayleighCoefficients, other.RayleighCoefficients, glm::vec3(epsilon));
		glm::bvec3 ozoneAbsorbtionEquals = glm::epsilonEqual(OzoneAbsorbtion, other.OzoneAbsorbtion, glm::vec3(epsilon));

		return glm::epsilonEqual(PlanetRadius, other.PlanetRadius, epsilon)
			&& glm::epsilonEqual(AtmosphereThickness, other.AtmosphereThickness, epsilon)
			&& glm::epsilonEqual(MieHeight, other.MieHeight, epsilon)
			&& glm::epsilonEqual(RayleighHeight, other.RayleighHeight, epsilon)
			&& glm::all(rayleighCoefficientsEqual)
			&& glm::epsilonEqual(RayleighAbsorbtion, other.RayleighAbsorbtion, epsilon)
			&& glm::epsilonEqual(MieCoefficient, other.MieCoefficient, epsilon)
			&& glm::epsilonEqual(MieAbsorbtion, other.MieAbsorbtion, epsilon)
			&& glm::all(ozoneAbsorbtionEquals);
	}

	bool AtmosphericScatteringParameters::operator!=(const AtmosphericScatteringParameters& other) const
	{
		return !operator==(other);
	}

	FLARE_IMPL_TYPE(Atmosphere);
	FLARE_SERIALIZABLE_IMPL(Atmosphere);

	void Atmosphere::RegisterRenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld)
	{
		FLARE_PROFILE_FUNCTION();
		if (!IsEnabled())
			return;

		RenderGraphTextureId sunTransmittanceLUT = renderGraph
			.GetResourceManager()
			.CreateFixedSizeTexture(TextureFormat::R32G32B32A32, glm::uvec2(SunTransmittanceLUTSize), "SunTransmittanceLUT");

		RenderGraphPassSpecifications lutPass{};
		lutPass.SetDebugName("SunTransmittanceLUTPass");
		lutPass.SetType(RenderGraphPassType::Graphics);
		lutPass.AddOutput(sunTransmittanceLUT);

		renderGraph.AddPass(lutPass, Ref<AtmosphereTransmittanceLUTPass>::New(Ref(this)));

		RenderGraphPassSpecifications mainPass{};
		mainPass.AddOutput(renderWorld.GetEntityComponent<const ViewportColorOutput>(viewportEntity).Id);
		mainPass.AddOutput(renderWorld.GetEntityComponent<const ViewportDepthOutput>(viewportEntity).Id);
		mainPass.AddInput(sunTransmittanceLUT);
		mainPass.SetType(RenderGraphPassType::Graphics);
		mainPass.SetDebugName("AtmospherePass");

		renderGraph.AddPass(mainPass, Ref<AtmospherePass>::New(sunTransmittanceLUT, Ref(this)));
	}

	const SerializableObjectDescriptor& Atmosphere::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(Atmosphere);
	}

	//
	// AtmosphereTransmittanceLUTPass
	//

	AtmosphereTransmittanceLUTPass::AtmosphereTransmittanceLUTPass(Ref<const Atmosphere> parameters)
		: m_Parameters(parameters)
	{
		FLARE_PROFILE_FUNCTION();

		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("AtmosphereSunTransmittanceLUT");
		if (shaderHandle.has_value())
		{
			m_SunTransmittanceMaterial = Material::Create(*shaderHandle);
		}

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		m_FrameInFlightFlags.resize(frameInFlightCount, false);
	}

	void AtmosphereTransmittanceLUTPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void AtmosphereTransmittanceLUTPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		bool lutIsDirty = m_Parameters->ScatteringParameters != m_PreviousScatteringParameters
			&& m_PreviousLUTSteps != m_Parameters->SunTransmittanceLUTSteps;

		if (lutIsDirty)
		{
			m_FrameInFlightFlags.assign(m_FrameInFlightFlags.size(), false);
		}

		uint32_t frameInFlight = GraphicsContext::GetInstance().GetCurrentFrameInFlight();

		if (m_FrameInFlightFlags[frameInFlight])
			return;

		m_FrameInFlightFlags[frameInFlight] = true;
		m_PreviousScatteringParameters = m_Parameters->ScatteringParameters;
		m_PreviousLUTSteps = m_Parameters->SunTransmittanceLUTSteps;

		Ref<Shader> shader = m_SunTransmittanceMaterial->GetShader();
		FLARE_CORE_ASSERT(shader);

		std::optional<uint32_t> mieCoefficientIndex = shader->GetPropertyIndex("u_Params.MieCoefficient");
		std::optional<uint32_t> mieAbsorbtionIndex = shader->GetPropertyIndex("u_Params.MieAbsorbtion");
		std::optional<uint32_t> rayleighAbsorbtionIndex = shader->GetPropertyIndex("u_Params.RayleighAbsorbtion");
		std::optional<uint32_t> rayleighCoefficientIndex = shader->GetPropertyIndex("u_Params.RayleighCoefficient");
		std::optional<uint32_t> ozoneAbsorbtionIndex = shader->GetPropertyIndex("u_Params.OzoneAbsorbtion");
		std::optional<uint32_t> planetRadius = shader->GetPropertyIndex("u_Params.PlanetRadius");
		std::optional<uint32_t>	atmosphereThickness = shader->GetPropertyIndex("u_Params.AtmosphereThickness");
		std::optional<uint32_t> sunTransmittanceSteps = shader->GetPropertyIndex("u_Params.SunTransmittanceSteps");
		std::optional<uint32_t> mieHeight = shader->GetPropertyIndex("u_Params.MieHeight");
		std::optional<uint32_t> rayleighHeight = shader->GetPropertyIndex("u_Params.RayleighHeight");

		m_SunTransmittanceMaterial->WritePropertyValue<float>(*mieCoefficientIndex, m_Parameters->ScatteringParameters.MieCoefficient);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*mieAbsorbtionIndex, m_Parameters->ScatteringParameters.MieAbsorbtion);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*rayleighAbsorbtionIndex, m_Parameters->ScatteringParameters.RayleighAbsorbtion);
		m_SunTransmittanceMaterial->WritePropertyValue<glm::vec3>(*rayleighCoefficientIndex, m_Parameters->ScatteringParameters.RayleighCoefficients);
		m_SunTransmittanceMaterial->WritePropertyValue<glm::vec3>(*ozoneAbsorbtionIndex, m_Parameters->ScatteringParameters.OzoneAbsorbtion);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*planetRadius, m_Parameters->ScatteringParameters.PlanetRadius);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*atmosphereThickness, m_Parameters->ScatteringParameters.AtmosphereThickness);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*mieHeight, m_Parameters->ScatteringParameters.MieHeight);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*rayleighHeight, m_Parameters->ScatteringParameters.RayleighHeight);
		m_SunTransmittanceMaterial->WritePropertyValue<int32_t>(*sunTransmittanceSteps, (int32_t)m_Parameters->SunTransmittanceLUTSteps);

		commandBuffer->ApplyMaterial(m_SunTransmittanceMaterial);
		commandBuffer->SetViewportAndScissors(Math::Rect(0, 0,
			(float)m_Parameters->SunTransmittanceLUTSize,
			(float)m_Parameters->SunTransmittanceLUTSize));

		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
	}

	//
	// AtmospherePass
	//

	AtmospherePass::AtmospherePass(RenderGraphTextureId sunTransmittanceLUT, Ref<const Atmosphere> parameters)
		: m_SunTransmittanceLUT(sunTransmittanceLUT), m_Parameters(parameters)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<AssetHandle> atmosphereShaderHandle = ShaderLibrary::FindShader("Atmosphere");
		if (atmosphereShaderHandle.has_value())
		{
			m_AtmosphereMaterial = Material::Create(*atmosphereShaderHandle);
		}

		SamplerSpecifications specifications{};
		specifications.Filter = TextureFiltering::Linear;
		specifications.WrapMode = TextureWrap::Clamp;
		m_LUTSampler = Sampler::Create(specifications);
	}

	void AtmospherePass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void AtmospherePass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<Shader> shader = m_AtmosphereMaterial->GetShader();

		std::optional<uint32_t> planetRadius = shader->GetPropertyIndex("u_Params.PlanetRadius");
		std::optional<uint32_t>	atmosphereThickness = shader->GetPropertyIndex("u_Params.AtmosphereThickness");
		std::optional<uint32_t> mieHeight = shader->GetPropertyIndex("u_Params.MieHeight");
		std::optional<uint32_t> rayleighHeight = shader->GetPropertyIndex("u_Params.RayleighHeight");
		std::optional<uint32_t> observerHeight = shader->GetPropertyIndex("u_Params.ObserverHeight");
		std::optional<uint32_t> viewRaySteps = shader->GetPropertyIndex("u_Params.ViewRaySteps");
		std::optional<uint32_t> sunTransmittanceSteps = shader->GetPropertyIndex("u_Params.SunTransmittanceSteps");

		std::optional<uint32_t> rayleighCoefficients = shader->GetPropertyIndex("u_Params.RayleighCoefficient");
		std::optional<uint32_t> rayleighAbsorbtion = shader->GetPropertyIndex("u_Params.RayleighAbsorbtion");
		std::optional<uint32_t> mieCoefficient = shader->GetPropertyIndex("u_Params.MieCoefficient");
		std::optional<uint32_t> mieAbsorbtion = shader->GetPropertyIndex("u_Params.MieAbsorbtion");
		std::optional<uint32_t> ozoneAbsorbtion = shader->GetPropertyIndex("u_Params.OzoneAbsorbtion");
		std::optional<uint32_t> groundColor = shader->GetPropertyIndex("u_Params.GroundColor");

		std::optional<uint32_t> sunTransmittanceLUT = shader->GetPropertyIndex("u_SunTransmittanceLUT");

		m_AtmosphereMaterial->WritePropertyValue<float>(*planetRadius, m_Parameters->ScatteringParameters.PlanetRadius);
		m_AtmosphereMaterial->WritePropertyValue<float>(*atmosphereThickness, m_Parameters->ScatteringParameters.AtmosphereThickness);
		m_AtmosphereMaterial->WritePropertyValue<float>(*mieHeight, m_Parameters->ScatteringParameters.MieHeight);
		m_AtmosphereMaterial->WritePropertyValue<float>(*rayleighHeight, m_Parameters->ScatteringParameters.RayleighHeight);
		m_AtmosphereMaterial->WritePropertyValue<float>(*observerHeight, m_Parameters->ObserverHeight);

		m_AtmosphereMaterial->WritePropertyValue(*rayleighCoefficients, m_Parameters->ScatteringParameters.RayleighCoefficients);
		m_AtmosphereMaterial->WritePropertyValue(*rayleighAbsorbtion, m_Parameters->ScatteringParameters.RayleighAbsorbtion);
		m_AtmosphereMaterial->WritePropertyValue(*mieCoefficient, m_Parameters->ScatteringParameters.MieCoefficient);
		m_AtmosphereMaterial->WritePropertyValue(*mieAbsorbtion, m_Parameters->ScatteringParameters.MieAbsorbtion);
		m_AtmosphereMaterial->WritePropertyValue(*ozoneAbsorbtion, m_Parameters->ScatteringParameters.OzoneAbsorbtion);
		m_AtmosphereMaterial->WritePropertyValue(*groundColor, m_Parameters->GroundColor);

		m_AtmosphereMaterial->WritePropertyValue<int32_t>(*viewRaySteps, (int32_t)m_Parameters->ViewRaySteps);
		m_AtmosphereMaterial->WritePropertyValue<int32_t>(*sunTransmittanceSteps, (int32_t)m_Parameters->SunTransmittanceSteps);
		m_AtmosphereMaterial->SetTextureProperty(*sunTransmittanceLUT, context.GetRenderGraph().GetTexture(m_SunTransmittanceLUT), m_LUTSampler);

		const ViewportGlobalResources& resources = context.RenderWorld.GetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);

		commandBuffer->SetGlobalDescriptorSet(resources.GetCurrentFrameResources().CameraDescriptorSet, 0);
		commandBuffer->SetGlobalDescriptorSet(resources.GetCurrentFrameResources().GlobalDescriptorSet, 1);
		commandBuffer->ApplyMaterial(m_AtmosphereMaterial);

		commandBuffer->SetDefaultViewportAndScissors();
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
	}
}
