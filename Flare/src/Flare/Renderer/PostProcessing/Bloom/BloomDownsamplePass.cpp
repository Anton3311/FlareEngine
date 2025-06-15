#include "PCH.h"
#include "BloomDownsamplePass.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Sampler.h"
#include "Flare/Renderer/ShaderLibrary.h"

namespace Flare
{
	BloomDownsamplePass::BloomDownsamplePass(RenderGraphTextureId sourceTexture, Ref<Sampler> sampler, bool reduceDynamicRange)
		: m_SourceTexture(sourceTexture), m_ReduceDynamicRange(reduceDynamicRange), m_Sampler(sampler)
	{
		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("BloomDownsample"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));
			m_Material = Material::Create(*shaderHandle);
		}
	}

	void BloomDownsamplePass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void BloomDownsamplePass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (m_Material == nullptr || m_Material->GetShader() == nullptr || !m_Material->GetShader()->IsLoaded())
			return;

		std::optional<uint32_t> colorTextureProperty = m_Material->GetShader()->GetPropertyIndex("u_Color");

		if (colorTextureProperty)
			m_Material->SetTextureProperty(*colorTextureProperty, context.GetRenderGraph().GetTexture(m_SourceTexture), m_Sampler);

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->SetDefaultViewportAndScissors();

		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
	}
}
