#include "PCH.h"
#include "BloomFilteringPass.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Sampler.h"
#include "Flare/Renderer/ShaderLibrary.h"

namespace Flare
{
	BloomFilteringPass::BloomFilteringPass(RenderGraphTextureId sourceTexture)
		: m_SourceTexture(sourceTexture)
	{
		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("BloomFilter"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));
			m_Material = Material::Create(*shaderHandle);
		}

		SamplerSpecifications specifications{};
		specifications.Filter = TextureFiltering::Linear;
		specifications.WrapMode = TextureWrap::Clamp;

		m_Sampler = Sampler::Create(specifications);
	}

	void BloomFilteringPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void BloomFilteringPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
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
