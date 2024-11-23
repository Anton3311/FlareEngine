#include "PCH.h"
#include "BloomBlitPass.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/PostProcessing/Bloom/Bloom.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/Sampler.h"
#include "Flare/Renderer/ShaderLibrary.h"

namespace Flare
{
	BloomBlitPass::BloomBlitPass(RenderGraphTextureId sourceTexture, Ref<const Bloom> parameters)
		: m_SourceTexture(sourceTexture), m_Parameters(parameters)
	{
		FLARE_PROFILE_FUNCTION();
		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("BloomBlit"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));
			m_Material = Material::Create(*shaderHandle);
		}

		SamplerSpecifications specifications{};
		specifications.Filter = TextureFiltering::Closest;
		specifications.WrapMode = TextureWrap::Clamp;

		m_Sampler = Sampler::Create(specifications);
	}

	void BloomBlitPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void BloomBlitPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		std::optional<uint32_t> colorTextureProperty = m_Material->GetShader()->GetPropertyIndex("u_Color");
		std::optional<uint32_t> intensityProperty = m_Material->GetShader()->GetPropertyIndex("u_Intensity");

		m_Material->SetTextureProperty(*colorTextureProperty, context.GetRenderGraph().GetTexture(m_SourceTexture), m_Sampler);
		m_Material->WritePropertyValue<float>(*intensityProperty, m_Parameters->Intensity);

		commandBuffer->SetDefaultViewportAndScissors();
		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
	}
}
