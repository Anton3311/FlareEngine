#include "PCH.h"

#include "Tonemapping.h"

#include "FlareCore/Log.h"

#include "FlareECS/World.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "Flare/Renderer/Passes/BlitPass.h"

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	FLARE_IMPL_TYPE(ToneMapping);
	FLARE_SERIALIZABLE_IMPL(ToneMapping);

	ToneMapping::ToneMapping()
	{
	}

	void ToneMapping::RegisterRenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld)
	{
		FLARE_PROFILE_FUNCTION();

		if (!IsEnabled())
			return;

		RenderGraphTextureId intermediateTexture = renderGraph.CreateTexture(TextureFormat::R11G11B10, "ToneMapping.IntermediateColorTexture");

		RenderGraphTextureId viewportColorTexture = renderWorld.GetEntityComponent<const ViewportColorOutput>(viewportEntity).Id;

		RenderGraphPassSpecifications toneMappingPass{};
		toneMappingPass.SetDebugName("ToneMapping");
		toneMappingPass.AddInput(viewportColorTexture);
		toneMappingPass.AddOutput(intermediateTexture);

		RenderGraphPassSpecifications blitPass{};
		blitPass.SetDebugName("ToneMappingBlit");
		BlitPass::ConfigureSpecifications(blitPass, intermediateTexture, viewportColorTexture);

		renderGraph.AddPass(toneMappingPass, Ref<ToneMappingPass>::New(viewportColorTexture));
		renderGraph.AddPass(blitPass, Ref<BlitPass>::New(intermediateTexture, viewportColorTexture, TextureFiltering::Closest));
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

	void ToneMappingPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void ToneMappingPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (m_Material == nullptr)
			return;

		auto colorTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_ScreenBuffer");
		if (colorTextureIndex)
			m_Material->SetTextureProperty(*colorTextureIndex, context.GetRenderGraphResourceManager().GetTexture(m_ColorTexture));

		commandBuffer->SetDefaultViewportAndScissors();

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
	}
}
