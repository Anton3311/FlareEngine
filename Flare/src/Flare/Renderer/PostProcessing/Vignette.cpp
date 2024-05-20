#include "Vignette.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/RendererPrimitives.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	FLARE_IMPL_TYPE(Vignette);

	static uint32_t s_ColorPropertyIndex = UINT32_MAX;
	static uint32_t s_RadiusPropertyIndex = UINT32_MAX;
	static uint32_t s_SmoothnessPropertyIndex = UINT32_MAX;

	Vignette::Vignette()
		: RenderPass(RenderPassQueue::PostProcessing), Enabled(false), Color(0.0f, 0.0f, 0.0f, 0.5f), Radius(1.0f), Smoothness(1.0f)
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("Vignette");
		if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			FLARE_CORE_ERROR("Vignette: Failed to find Vignette shader");
			return;
		}

		Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderHandle.value());
		m_Material = Material::Create(shader);

		s_ColorPropertyIndex = shader->GetPropertyIndex("u_Params.Color").value_or(UINT32_MAX);
		s_RadiusPropertyIndex = shader->GetPropertyIndex("u_Params.Radius").value_or(UINT32_MAX);
		s_SmoothnessPropertyIndex = shader->GetPropertyIndex("u_Params.Smoothness").value_or(UINT32_MAX);
	}

	void Vignette::OnRender(RenderingContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		if (!Enabled || !Renderer::GetCurrentViewport().PostProcessingEnabled)
			return;

		m_Material->WritePropertyValue(s_ColorPropertyIndex, Color);
		m_Material->WritePropertyValue(s_RadiusPropertyIndex, Radius);
		m_Material->WritePropertyValue(s_SmoothnessPropertyIndex, Smoothness);

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();
		commandBuffer->BeginRenderTarget(context.RenderTarget);

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

			vulkanCommandBuffer->SetPrimaryDescriptorSet(nullptr);
			vulkanCommandBuffer->SetSecondaryDescriptorSet(nullptr);
		}

		commandBuffer->ApplyMaterial(m_Material);

		const auto& frameBufferSpec = context.RenderTarget->GetSpecifications();
		commandBuffer->SetViewportAndScisors(Math::Rect(0.0f, 0.0f, (float)frameBufferSpec.Width, (float)frameBufferSpec.Height));

		commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

		commandBuffer->EndRenderTarget();
	}

	void TypeSerializer<Vignette>::OnSerialize(Vignette& vignette, SerializationStream& stream)
	{
		stream.Serialize("Enabled", SerializationValue(vignette.Enabled));
		stream.Serialize("Color", SerializationValue(vignette.Color, SerializationValueFlags::Color));
		stream.Serialize("Radius", SerializationValue(vignette.Radius));
		stream.Serialize("Smoothness", SerializationValue(vignette.Smoothness));
	}



	VignettePass::VignettePass()
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("Vignette");
		if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			FLARE_CORE_ERROR("Vignette: Failed to find Vignette shader");
			return;
		}

		Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderHandle.value());
		m_Material = Material::Create(shader);
	}

	void VignettePass::OnRender(Ref<CommandBuffer> commandBuffer)
	{
		Ref<Shader> shader = m_Material->GetShader();

		auto colorPropertyIndex = shader->GetPropertyIndex("u_Params.Color");
		auto radiusPropertyIndex = shader->GetPropertyIndex("u_Params.Radius");
		auto smoothnessPropertyIndex = shader->GetPropertyIndex("u_Params.Smoothness");

		if (colorPropertyIndex)
			m_Material->WritePropertyValue(*colorPropertyIndex, m_Color);
		if (radiusPropertyIndex)
			m_Material->WritePropertyValue(*radiusPropertyIndex, m_Radius);
		if (smoothnessPropertyIndex)
			m_Material->WritePropertyValue(*smoothnessPropertyIndex, m_Smoothness);

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
	}
}
