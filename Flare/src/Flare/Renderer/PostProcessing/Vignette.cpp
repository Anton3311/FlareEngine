#include "Vignette.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/RendererPrimitives.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	FLARE_IMPL_TYPE(Vignette);

	static uint32_t s_ColorPropertyIndex = UINT32_MAX;
	static uint32_t s_RadiusPropertyIndex = UINT32_MAX;
	static uint32_t s_SmoothnessPropertyIndex = UINT32_MAX;

	Vignette::Vignette()
		: Color(0.0f, 0.0f, 0.0f, 0.5f), Radius(1.0f), Smoothness(1.0f)
	{
	}

	void Vignette::RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		if (!IsEnabled())
			return;
		
		RenderGraphPassSpecifications specifications{};
		specifications.SetDebugName("VignettePass");
		specifications.AddOutput(viewport.ColorTexture, 0);

		renderGraph.AddPass(specifications, CreateRef<VignettePass>());
	}

	const SerializableObjectDescriptor& Vignette::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(Vignette);
	}

	void TypeSerializer<Vignette>::OnSerialize(Vignette& vignette, SerializationStream& stream)
	{
		stream.Serialize("Color", SerializationValue(vignette.Color, SerializationValueFlags::Color));
		stream.Serialize("Radius", SerializationValue(vignette.Radius));
		stream.Serialize("Smoothness", SerializationValue(vignette.Smoothness));
	}



	VignettePass::VignettePass()
	{
		auto vignette = Scene::GetActive()->GetPostProcessingManager().GetEffect<Vignette>();
		FLARE_CORE_ASSERT(vignette);

		m_Parameters = *vignette;

		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("Vignette");
		if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			FLARE_CORE_ERROR("Vignette: Failed to find Vignette shader");
			return;
		}

		Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderHandle.value());
		m_Material = Material::Create(shader);
	}

	void VignettePass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		Ref<Shader> shader = m_Material->GetShader();

		auto colorPropertyIndex = shader->GetPropertyIndex("u_Params.Color");
		auto radiusPropertyIndex = shader->GetPropertyIndex("u_Params.Radius");
		auto smoothnessPropertyIndex = shader->GetPropertyIndex("u_Params.Smoothness");

		if (colorPropertyIndex)
			m_Material->WritePropertyValue(*colorPropertyIndex, m_Parameters->Color);
		if (radiusPropertyIndex)
			m_Material->WritePropertyValue(*radiusPropertyIndex, m_Parameters->Radius);
		if (smoothnessPropertyIndex)
			m_Material->WritePropertyValue(*smoothnessPropertyIndex, m_Parameters->Smoothness);

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

		commandBuffer->EndRenderTarget();
	}
}
