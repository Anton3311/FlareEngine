#include "PCH.h"
#include "BloomLuminanceIsolationPass.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/PostProcessing/Bloom/Bloom.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/RenderGraph/RenderGraph.h"

namespace Flare
{
	BloomLuminanceIsolationPass::BloomLuminanceIsolationPass(RenderGraphTextureId colorTexture, Ref<const Bloom> parameters)
		: m_Parameters(parameters), m_ColorTexture(colorTexture)
	{
		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("BloomLuminance"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));
			m_Material = Material::Create(*shaderHandle);
		}
	}

	void BloomLuminanceIsolationPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void BloomLuminanceIsolationPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		std::optional<uint32_t> colorProperty = m_Material->GetShader()->GetPropertyIndex("u_HDRColor");
		std::optional<uint32_t> thresholdProperty = m_Material->GetShader()->GetPropertyIndex("u_Threshold");

		m_Material->SetTextureProperty(*colorProperty, context.GetRenderGraph().GetTexture(m_ColorTexture));
		m_Material->WritePropertyValue<float>(*thresholdProperty, m_Parameters->Threshold);

		commandBuffer->SetDefaultViewportAndScissors();
		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
	}
}
