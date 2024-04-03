#include "AtmospherePass.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererPrimitives.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"

namespace Flare
{
	FLARE_IMPL_TYPE(AtmospherePass);

	void AtmospherePass::OnRender(RenderingContext& context)
	{
		if (!AtmosphereMaterial || !Enabled || !Renderer::GetCurrentViewport().PostProcessingEnabled)
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

#if 1
		AtmosphereMaterial->WritePropertyValue<float>(*planetRadius, PlanetRadius);
		AtmosphereMaterial->WritePropertyValue<float>(*atmosphereThickness, AtmosphereThickness);
		AtmosphereMaterial->WritePropertyValue<float>(*mieHeight, MieHeight);
		AtmosphereMaterial->WritePropertyValue<float>(*rayleighHeight, RayleighHeight);
		AtmosphereMaterial->WritePropertyValue<float>(*observerHeight, ObserverHeight);

		AtmosphereMaterial->WritePropertyValue<int32_t>(*viewRaySteps, (int32_t)ViewRaySteps);
		AtmosphereMaterial->WritePropertyValue<int32_t>(*sunTransmittanceSteps, (int32_t)SunTransmittanceSteps);
#endif

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().GetPrimaryCommandBuffer();
			Ref<VulkanFrameBuffer> renderTarget = As<VulkanFrameBuffer>(context.RenderTarget);

			commandBuffer->BeginRenderPass(renderTarget->GetCompatibleRenderPass(), renderTarget);
			commandBuffer->SetPrimaryDescriptorSet(Renderer::GetPrimaryDescriptorSet());
			commandBuffer->SetSecondaryDescriptorSet(nullptr);
			commandBuffer->ApplyMaterial(AtmosphereMaterial);
			commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
			commandBuffer->EndRenderPass();
			return;
		}

		Renderer::DrawFullscreenQuad(AtmosphereMaterial);
	}
}
