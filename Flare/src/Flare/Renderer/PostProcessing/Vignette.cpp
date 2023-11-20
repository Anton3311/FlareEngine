#include "Vignette.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/ShaderLibrary.h"

namespace Flare
{
	FLARE_IMPL_TYPE(Vignette,
		FLARE_PROPERTY(Vignette, Enabled),
		FLARE_PROPERTY(Vignette, Color),
		FLARE_PROPERTY(Vignette, Radius),
		FLARE_PROPERTY(Vignette, Smoothness),
	);

	static uint32_t s_ColorPropertyIndex = UINT32_MAX;
	static uint32_t s_RadiusPropertyIndex = UINT32_MAX;
	static uint32_t s_SmoothnessPropertyIndex = UINT32_MAX;

	Vignette::Vignette()
		: Enabled(false), Color(0.0f, 0.0f, 0.0f, 0.5f), Radius(1.0f), Smoothness(1.0f)
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("Vignette");
		if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			FLARE_CORE_ERROR("Vignette: Failed to find Vignette shader");
			return;
		}

		Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderHandle.value());
		m_Material = CreateRef<Material>(shader);

		s_ColorPropertyIndex = shader->GetPropertyIndex("u_Params.Color").value_or(UINT32_MAX);
		s_RadiusPropertyIndex = shader->GetPropertyIndex("u_Params.Radius").value_or(UINT32_MAX);
		s_SmoothnessPropertyIndex = shader->GetPropertyIndex("u_Params.Smoothness").value_or(UINT32_MAX);
	}

	void Vignette::OnRender(RenderingContext& context)
	{
		if (!Enabled)
			return;

		FrameBufferAttachmentsMask writeMask = context.RenderTarget->GetWriteMask();

		m_Material->WritePropertyValue(s_ColorPropertyIndex, Color);
		m_Material->WritePropertyValue(s_RadiusPropertyIndex, Radius);
		m_Material->WritePropertyValue(s_SmoothnessPropertyIndex, Smoothness);

		context.RenderTarget->SetWriteMask(0x1);
		Renderer::DrawFullscreenQuad(m_Material);
		context.RenderTarget->SetWriteMask(writeMask);
	}
}
