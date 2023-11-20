#include "Tonemapping.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/ShaderLibrary.h"

namespace Flare
{
	FLARE_IMPL_TYPE(ToneMapping, FLARE_PROPERTY(ToneMapping, Enabled));

	ToneMapping::ToneMapping()
		: Enabled(false)
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("AcesToneMapping");
		if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			FLARE_CORE_ERROR("ToneMapping: Failed to find ToneMapping shader");
			return;
		}

		m_Material = CreateRef<Material>(AssetManager::GetAsset<Shader>(shaderHandle.value()));
	}

	void ToneMapping::OnRender(RenderingContext& context)
	{
		if (!Enabled)
			return;

		Ref<FrameBuffer> output = context.RTPool.Get();

		context.RenderTarget->BindAttachmentTexture(0, 0);
		output->Bind();

		Renderer::DrawFullscreenQuad(m_Material);

		context.RenderTarget->Blit(output, 0, 0);
		context.RTPool.Release(output);
		context.RenderTarget->Bind();
	}
}
