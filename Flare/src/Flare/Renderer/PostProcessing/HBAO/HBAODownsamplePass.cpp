#include "HBAODownsamplePass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Viewport.h"

namespace Flare
{
	HBAODownsamplePass::HBAODownsamplePass(RenderGraphTextureId depthTexture)
		: m_DepthTexture(depthTexture)
	{
		FLARE_PROFILE_FUNCTION();

		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("HBAODownsample"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));

			m_Material = Material::Create(*shaderHandle);
		}
	}

	void HBAODownsamplePass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
	}

	void HBAODownsamplePass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		commandBuffer->SetDefaultViewportAndScissors();

		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GetFrameResources().CameraDescriptorSet, 0);

		std::optional<uint32_t> depthTextureProperty = m_Material->GetShader()->GetPropertyIndex("u_DepthTexture");

		if (!depthTextureProperty)
			return;

		m_Material->SetTextureProperty(*depthTextureProperty, context.GetRenderGraphResourceManager().GetTexture(m_DepthTexture));

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 1);
	}
}
