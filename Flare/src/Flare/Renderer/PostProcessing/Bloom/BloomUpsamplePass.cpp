#include "PCH.h"
#include "BloomUpsamplePass.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/PostProcessing/Bloom/Bloom.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/RenderGraph/RenderGraph.h"
#include "Flare/Renderer/Sampler.h"
#include "Flare/Renderer/ShaderLibrary.h"

namespace Flare
{
	BloomUpsamplePass::BloomUpsamplePass(Ref<const Bloom> parameters, Ref<Sampler> sampler, RenderGraphTextureId sourceTexture)
		: m_SourceTexture(sourceTexture), m_Parameters(parameters), m_Sampler(sampler)
	{
		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("BloomUpsample"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));
			m_Material = Material::Create(*shaderHandle);
		}
	}

	void BloomUpsamplePass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void BloomUpsamplePass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		if (m_Material == nullptr || m_Material->GetShader() == nullptr || !m_Material->GetShader()->IsLoaded())
			return;

		std::optional<uint32_t> texelSizeProperty = m_Material->GetShader()->GetPropertyIndex("u_TexelSize");
		std::optional<uint32_t> colorTextureProperty = m_Material->GetShader()->GetPropertyIndex("u_Color");

		Ref<Texture> sourceTexture = context.GetRenderGraph().GetTexture(m_SourceTexture);
		const TextureSpecifications& specifications = sourceTexture->GetSpecifications();

		glm::vec2 texelSize = glm::vec2(m_Parameters->Radius) / glm::vec2((float)specifications.Width, (float)specifications.Height);
		m_Material->WritePropertyValue<glm::vec2>(*texelSizeProperty, texelSize);
		m_Material->SetTextureProperty(*colorTextureProperty, sourceTexture, m_Sampler);

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->SetDefaultViewportAndScissors();

		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
	}
}
