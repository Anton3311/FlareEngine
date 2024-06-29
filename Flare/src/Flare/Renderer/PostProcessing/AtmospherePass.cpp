#include "AtmospherePass.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/RendererPrimitives.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	FLARE_IMPL_TYPE(AtmospherePass);

	AtmospherePass::AtmospherePass()
		: RenderPass(RenderPassQueue::PostProcessing)
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("AtmosphereSunTransmittanceLUT");
		if (shaderHandle.has_value())
		{
			m_SunTransmittanceMaterial = Material::Create(*shaderHandle);
		}
	}

	void AtmospherePass::OnRender(RenderingContext& context)
	{
		if (!AtmosphereMaterial || !Enabled || !Renderer::GetCurrentViewport().IsPostProcessingEnabled())
			return;

		Ref<Shader> shader = AtmosphereMaterial->GetShader();
		if (shader == nullptr)
			return;

		std::optional<uint32_t> planetRadius = shader->GetPropertyIndex("u_Params.PlanetRadius");
		std::optional<uint32_t>	atmosphereThickness = shader->GetPropertyIndex("u_Params.AtmosphereThickness");
		std::optional<uint32_t> mieHeight = shader->GetPropertyIndex("u_Params.MieHeight");
		std::optional<uint32_t> rayleighHeight = shader->GetPropertyIndex("u_Params.RayleighHeight");
		std::optional<uint32_t> observerHeight = shader->GetPropertyIndex("u_Params.ObserverHeight");
		std::optional<uint32_t> viewRaySteps = shader->GetPropertyIndex("u_Params.ViewRaySteps");
		std::optional<uint32_t> sunTransmittanceSteps = shader->GetPropertyIndex("u_Params.SunTransmittanceSteps");

		std::optional<uint32_t> sunTransmittanceLUT = shader->GetPropertyIndex("u_SunTransmittanceLUT");

		AtmosphereMaterial->WritePropertyValue<float>(*planetRadius, PlanetRadius);
		AtmosphereMaterial->WritePropertyValue<float>(*atmosphereThickness, AtmosphereThickness);
		AtmosphereMaterial->WritePropertyValue<float>(*mieHeight, MieHeight);
		AtmosphereMaterial->WritePropertyValue<float>(*rayleighHeight, RayleighHeight);
		AtmosphereMaterial->WritePropertyValue<float>(*observerHeight, ObserverHeight);

		AtmosphereMaterial->WritePropertyValue<int32_t>(*viewRaySteps, (int32_t)ViewRaySteps);
		AtmosphereMaterial->WritePropertyValue<int32_t>(*sunTransmittanceSteps, (int32_t)SunTransmittanceSteps);

		if (sunTransmittanceLUT.has_value() && m_SunTransmittanceLUT != nullptr)
		{
			AtmosphereMaterial->SetTextureProperty(*sunTransmittanceLUT, m_SunTransmittanceLUT->GetAttachment(0));
		}

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();

		if (m_SunTransmittanceMaterial != nullptr)
		{
			GenerateSunTransmittanceLUT(commandBuffer);
		}

		commandBuffer->BeginRenderTarget(context.RenderTarget);

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().GetPrimaryCommandBuffer();

			commandBuffer->SetPrimaryDescriptorSet(Renderer::GetPrimaryDescriptorSet());
			commandBuffer->SetSecondaryDescriptorSet(nullptr);
		}

		commandBuffer->ApplyMaterial(AtmosphereMaterial);

		const auto& frameBufferSpec = context.RenderTarget->GetSpecifications();
		commandBuffer->SetViewportAndScisors(Math::Rect(0.0f, 0.0f, (float)frameBufferSpec.Width, (float)frameBufferSpec.Height));

		commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
		commandBuffer->EndRenderTarget();
	}

	void AtmospherePass::RegenerateSunTransmittanceLUT()
	{
		m_SunTransmittanceLUTIsDirty = true;
	}

	void AtmospherePass::GenerateSunTransmittanceLUT(Ref<CommandBuffer> commandBuffer)
	{
		if (!m_SunTransmittanceLUTIsDirty)
			return;

		m_SunTransmittanceLUTIsDirty = false;

		if (m_SunTransmittanceLUT == nullptr)
		{
			FrameBufferSpecifications lutSpecifications{};
			lutSpecifications.Width = SunTransmittanceLUTSize;
			lutSpecifications.Height = SunTransmittanceLUTSize;
			lutSpecifications.Attachments.emplace_back(FrameBufferAttachmentSpecifications{
				TextureFormat::R32G32B32A32,
				TextureWrap::Clamp,
				TextureFiltering::Linear
			});

			m_SunTransmittanceLUT = FrameBuffer::Create(lutSpecifications);
		}

		if (m_SunTransmittanceLUT->GetSpecifications().Width != SunTransmittanceLUTSize)
		{
			m_SunTransmittanceLUT->Resize(SunTransmittanceLUTSize, SunTransmittanceLUTSize);
		}

		Ref<Shader> shader = m_SunTransmittanceMaterial->GetShader();
		FLARE_CORE_ASSERT(shader);

		Ref<Shader> atmosphereShader = AtmosphereMaterial->GetShader();
		std::optional<uint32_t> mieCoefficientIndex1 = atmosphereShader->GetPropertyIndex("u_Params.MieCoefficient");
		std::optional<uint32_t> mieAbsorbtionIndex1 = atmosphereShader->GetPropertyIndex("u_Params.MieAbsorbtion");
		std::optional<uint32_t> rayleighAbsorbtionIndex1 = atmosphereShader->GetPropertyIndex("u_Params.RayleighAbsorbtion");
		std::optional<uint32_t> rayleighCoefficientIndex1 = atmosphereShader->GetPropertyIndex("u_Params.RayleighCoefficient");
		std::optional<uint32_t> ozoneAbsorbtionIndex1 = atmosphereShader->GetPropertyIndex("u_Params.OzoneAbsorbtion");

		std::optional<uint32_t> mieCoefficientIndex2 = shader->GetPropertyIndex("u_Params.MieCoefficient");
		std::optional<uint32_t> mieAbsorbtionIndex2 = shader->GetPropertyIndex("u_Params.MieAbsorbtion");
		std::optional<uint32_t> rayleighAbsorbtionIndex2 = shader->GetPropertyIndex("u_Params.RayleighAbsorbtion");
		std::optional<uint32_t> rayleighCoefficientIndex2 = shader->GetPropertyIndex("u_Params.RayleighCoefficient");
		std::optional<uint32_t> ozoneAbsorbtionIndex2 = shader->GetPropertyIndex("u_Params.OzoneAbsorbtion");

		m_SunTransmittanceMaterial->WritePropertyValue<float>(*mieCoefficientIndex2, AtmosphereMaterial->ReadPropertyValue<float>(*mieCoefficientIndex1));
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*mieAbsorbtionIndex2, AtmosphereMaterial->ReadPropertyValue<float>(*mieAbsorbtionIndex1));
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*rayleighAbsorbtionIndex2, AtmosphereMaterial->ReadPropertyValue<float>(*rayleighAbsorbtionIndex1));
		m_SunTransmittanceMaterial->WritePropertyValue<glm::vec3>(*rayleighCoefficientIndex2, AtmosphereMaterial->ReadPropertyValue<glm::vec3>(*rayleighCoefficientIndex1));
		m_SunTransmittanceMaterial->WritePropertyValue<glm::vec3>(*ozoneAbsorbtionIndex2, AtmosphereMaterial->ReadPropertyValue<glm::vec3>(*ozoneAbsorbtionIndex1));

		std::optional<uint32_t> planetRadius = shader->GetPropertyIndex("u_Params.PlanetRadius");
		std::optional<uint32_t>	atmosphereThickness = shader->GetPropertyIndex("u_Params.AtmosphereThickness");
		std::optional<uint32_t> sunTransmittanceSteps = shader->GetPropertyIndex("u_Params.SunTransmittanceSteps");

		std::optional<uint32_t> mieHeight = shader->GetPropertyIndex("u_Params.MieHeight");
		std::optional<uint32_t> rayleighHeight = shader->GetPropertyIndex("u_Params.RayleighHeight");

		m_SunTransmittanceMaterial->WritePropertyValue<float>(*planetRadius, PlanetRadius);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*atmosphereThickness, AtmosphereThickness);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*mieHeight, MieHeight);
		m_SunTransmittanceMaterial->WritePropertyValue<float>(*rayleighHeight, RayleighHeight);
		m_SunTransmittanceMaterial->WritePropertyValue<int32_t>(*sunTransmittanceSteps, (int32_t)SunTransmittanceLUTSteps);

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
		Ref<VulkanFrameBuffer> lut = As<VulkanFrameBuffer>(m_SunTransmittanceLUT);
		VkImage lutImage = lut->GetAttachmentImage(0);

		vulkanCommandBuffer->TransitionImageLayout(lutImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		vulkanCommandBuffer->BeginRenderTarget(m_SunTransmittanceLUT);
		vulkanCommandBuffer->ApplyMaterial(m_SunTransmittanceMaterial);
		vulkanCommandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
		vulkanCommandBuffer->EndRenderTarget();

		vulkanCommandBuffer->TransitionImageLayout(lutImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}
