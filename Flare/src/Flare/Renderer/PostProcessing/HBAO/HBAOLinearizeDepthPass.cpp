#include "PCH.h"

#include "HBAOLinearizeDepthPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/World.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/ComputeShader.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/ShaderLibrary.h"

namespace Flare
{
	HBAOLinearizeDepthPass::HBAOLinearizeDepthPass(RenderGraphTextureId depthTexture, RenderGraphTextureId linearDepthTexture)
		: m_DepthTexture(depthTexture), m_LinearDepthTexture(linearDepthTexture)
	{
		FLARE_PROFILE_FUNCTION();

		if (std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("HBAOLinearizeDepth"))
		{
			FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));

			m_ComputeShader = AssetManager::GetAsset<ComputeShader>(*shaderHandle);
			
			m_ConstantBuffer.SetShader(m_ComputeShader);
			m_DescriptorBuffer.SetShader(m_ComputeShader);
		}
	}

	void HBAOLinearizeDepthPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
	}

	void HBAOLinearizeDepthPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const ViewportGlobalResources* viewportResources = context.RenderWorld.TryGetEntityComponent<const ViewportGlobalResources>(context.ViewportEntity);
		FLARE_CORE_ASSERT(viewportResources);

		commandBuffer->SetGlobalDescriptorSet(viewportResources->GetCurrentFrameResources().CameraDescriptorSet, 0);

		Ref<const ComputeShaderMetadata> metadata = m_ComputeShader->GetMetadata();
		std::optional<size_t> depthTextureProperty = metadata->FindDescriptorProperty("u_DepthTexture");
		std::optional<size_t> linearDepthTextureProperty = metadata->FindDescriptorProperty("u_LinearDepth");
		std::optional<size_t> imageSizeProperty = metadata->FindConstantProperty("u_ImageSize");

		if (!depthTextureProperty || !linearDepthTextureProperty)
			return;

		Ref<Texture> depthTexture = context.GetRenderGraphResourceManager().GetTexture(m_DepthTexture);
		glm::uvec2 textureSize = depthTexture->GetSize();

		m_DescriptorBuffer.SetTexture(*depthTextureProperty, depthTexture);
		m_DescriptorBuffer.SetTexture(*linearDepthTextureProperty, context.GetRenderGraphResourceManager().GetTexture(m_LinearDepthTexture));

		m_ConstantBuffer.SetProperty<glm::ivec2>(*imageSizeProperty, textureSize);

		commandBuffer->BindComputeShader(m_ComputeShader);
		commandBuffer->PushConstants(m_ConstantBuffer);
		commandBuffer->PushDescriptorProperties(m_DescriptorBuffer);

		glm::uvec2 groupSize = metadata->LocalGroupSize;
		glm::uvec2 groupCount = (textureSize + groupSize - glm::uvec2(1, 1)) / groupSize;

		commandBuffer->DispatchCompute(glm::uvec3(groupCount, 1));
	}
}
