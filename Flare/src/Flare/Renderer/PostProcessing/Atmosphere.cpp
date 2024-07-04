#include "Atmosphere.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/Material.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	FLARE_IMPL_TYPE(Atmosphere);
	void Atmosphere::RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		if (!IsEnabled())
			return;

		RenderGraphPassSpecifications specifications{};
		specifications.AddOutput(viewport.ColorTexture, 0);
		specifications.AddOutput(viewport.DepthTexture, 1);
		specifications.SetType(RenderGraphPassType::Graphics);
		specifications.SetDebugName("AtmospherePass");

		renderGraph.AddPass(specifications, CreateRef<AtmospherePass>());
	}

	const SerializableObjectDescriptor& Atmosphere::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(Atmosphere);
	}

	AtmospherePass::AtmospherePass()
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("AtmosphereSunTransmittanceLUT");
		if (shaderHandle.has_value())
		{
			m_SunTransmittanceMaterial = Material::Create(*shaderHandle);
		}

		std::optional<AssetHandle> atmosphereShaderHandle = ShaderLibrary::FindShader("Atmosphere");
		if (atmosphereShaderHandle.has_value())
		{
			m_AtmosphereMaterial = Material::Create(*atmosphereShaderHandle);
		}

		auto result = Scene::GetActive()->GetPostProcessingManager().GetEffect<Atmosphere>();
		FLARE_CORE_ASSERT(result.has_value());
		m_Parameters = *result;
	}

	void AtmospherePass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
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

		m_AtmosphereMaterial->WritePropertyValue<float>(*planetRadius, m_Parameters->PlanetRadius);
		m_AtmosphereMaterial->WritePropertyValue<float>(*atmosphereThickness, m_Parameters->AtmosphereThickness);
		m_AtmosphereMaterial->WritePropertyValue<float>(*mieHeight, m_Parameters->MieHeight);
		m_AtmosphereMaterial->WritePropertyValue<float>(*rayleighHeight, m_Parameters->RayleighHeight);
		m_AtmosphereMaterial->WritePropertyValue<float>(*observerHeight, m_Parameters->ObserverHeight);

		m_AtmosphereMaterial->WritePropertyValue(*rayleighCoefficients, m_Parameters->RayleighCoefficients);
		m_AtmosphereMaterial->WritePropertyValue(*rayleighAbsorbtion, m_Parameters->RayleighAbsorbtion);
		m_AtmosphereMaterial->WritePropertyValue(*mieCoefficient, m_Parameters->MieCoefficient);
		m_AtmosphereMaterial->WritePropertyValue(*mieAbsorbtion, m_Parameters->MieAbsorbtion);
		m_AtmosphereMaterial->WritePropertyValue(*ozoneAbsorbtion, m_Parameters->OzoneAbsorbtion);
		m_AtmosphereMaterial->WritePropertyValue(*groundColor, m_Parameters->GroundColor);

		m_AtmosphereMaterial->WritePropertyValue<int32_t>(*viewRaySteps, (int32_t)m_Parameters->ViewRaySteps);
		m_AtmosphereMaterial->WritePropertyValue<int32_t>(*sunTransmittanceSteps, (int32_t)m_Parameters->SunTransmittanceSteps);

		if (m_SunTransmittanceMaterial != nullptr)
		{
			GenerateSunTransmittanceLUT(commandBuffer);
		}

		if (sunTransmittanceLUT.has_value() && m_SunTransmittanceLUT != nullptr)
		{
			m_AtmosphereMaterial->SetTextureProperty(*sunTransmittanceLUT, m_SunTransmittanceLUT->GetAttachment(0));
		}

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().GetPrimaryCommandBuffer();

			commandBuffer->SetPrimaryDescriptorSet(Renderer::GetPrimaryDescriptorSet());
			commandBuffer->SetSecondaryDescriptorSet(nullptr);
		}

		commandBuffer->ApplyMaterial(m_AtmosphereMaterial);

		const auto& renderTargetSpecifications = context.GetRenderTarget()->GetSpecifications();
		commandBuffer->SetViewportAndScisors(Math::Rect(0.0f, 0.0f, (float)renderTargetSpecifications.Width, (float)renderTargetSpecifications.Height));

		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
		commandBuffer->EndRenderTarget();
	}

	void AtmospherePass::GenerateSunTransmittanceLUT(Ref<CommandBuffer> commandBuffer)
	{
		if (m_SunTransmittanceLUT == nullptr)
		{
			FrameBufferSpecifications lutSpecifications{};
			lutSpecifications.Width = m_Parameters->SunTransmittanceLUTSize;
			lutSpecifications.Height = m_Parameters->SunTransmittanceLUTSize;
			lutSpecifications.Attachments.emplace_back(FrameBufferAttachmentSpecifications{
				TextureFormat::R32G32B32A32,
				TextureWrap::Clamp,
				TextureFiltering::Linear
			});

			m_SunTransmittanceLUT = FrameBuffer::Create(lutSpecifications);
		}

		if (m_SunTransmittanceLUT->GetSpecifications().Width == m_Parameters->SunTransmittanceLUTSize)
		{
			return;
		}

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

		m_SunTransmittanceMaterial->WritePropertyValue<float>(*mieCoefficientIndex, m_Parameters->MieCoefficient);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*mieAbsorbtionIndex, m_Parameters->MieAbsorbtion);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*rayleighAbsorbtionIndex, m_Parameters->RayleighAbsorbtion);
		m_SunTransmittanceMaterial->WritePropertyValue<glm::vec3>(*rayleighCoefficientIndex, m_Parameters->RayleighCoefficients);
		m_SunTransmittanceMaterial->WritePropertyValue<glm::vec3>(*ozoneAbsorbtionIndex, m_Parameters->OzoneAbsorbtion);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*planetRadius, m_Parameters->PlanetRadius);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*atmosphereThickness, m_Parameters->AtmosphereThickness);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*mieHeight, m_Parameters->MieHeight);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*rayleighHeight, m_Parameters->RayleighHeight);
		m_SunTransmittanceMaterial->WritePropertyValue<int32_t>(*sunTransmittanceSteps, (int32_t)m_Parameters->SunTransmittanceLUTSteps);

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
		Ref<VulkanFrameBuffer> lut = As<VulkanFrameBuffer>(m_SunTransmittanceLUT);
		VkImage lutImage = lut->GetAttachmentImage(0);

		vulkanCommandBuffer->TransitionImageLayout(lutImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		vulkanCommandBuffer->BeginRenderTarget(m_SunTransmittanceLUT);
		vulkanCommandBuffer->ApplyMaterial(m_SunTransmittanceMaterial);

		vulkanCommandBuffer->SetViewportAndScisors(Math::Rect(0, 0,
			(float)m_Parameters->SunTransmittanceLUTSize,
			(float)m_Parameters->SunTransmittanceLUTSize));

		vulkanCommandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
		vulkanCommandBuffer->EndRenderTarget();

		vulkanCommandBuffer->TransitionImageLayout(lutImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}
