#include "Tonemapping.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/Passes/BlitPass.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	FLARE_IMPL_TYPE(ToneMapping);

	ToneMapping::ToneMapping()
	{
	}

	void ToneMapping::RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		if (!IsEnabled())
			return;

		RenderGraphTextureId intermediateTexture = renderGraph.CreateTexture(TextureFormat::R11G11B10, "ToneMapping.IntermediateColorTexture");

		RenderGraphPassSpecifications toneMappingPass{};
		toneMappingPass.SetDebugName("ToneMapping");
		toneMappingPass.AddInput(viewport.ColorTextureId);
		toneMappingPass.AddOutput(intermediateTexture, 0);

		RenderGraphPassSpecifications blitPass{};
		blitPass.SetDebugName("ToneMappingBlit");
		BlitPass::ConfigureSpecifications(blitPass, intermediateTexture, viewport.ColorTextureId);

		renderGraph.AddPass(toneMappingPass, CreateRef<ToneMappingPass>(viewport.ColorTextureId));
		renderGraph.AddPass(blitPass, CreateRef<BlitPass>(intermediateTexture, viewport.ColorTextureId, TextureFiltering::Closest));
	}

	const SerializableObjectDescriptor& ToneMapping::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(ToneMapping);
	}



	ToneMappingPass::ToneMappingPass(RenderGraphTextureId colorTexture)
		: m_ColorTexture(colorTexture)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("AcesToneMapping");
		if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			FLARE_CORE_ERROR("ToneMapping: Failed to find ToneMapping shader");
			return;
		}

		Ref<Shader> shader = AssetManager::GetAsset<Shader>(shaderHandle.value());
		m_Material = Material::Create(shader);
	}

	void ToneMappingPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (m_Material == nullptr)
			return;

		auto colorTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_ScreenBuffer");
		if (colorTextureIndex)
			m_Material->SetTextureProperty(*colorTextureIndex, context.GetRenderGraphResourceManager().GetTexture(m_ColorTexture));

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());
		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f, 0.0f), (glm::vec2)context.GetViewport().GetSize()));

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);

		commandBuffer->EndRenderTarget();
	}
}
