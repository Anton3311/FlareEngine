#include "SSAO.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "Flare/AssetManager/AssetManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include <random>

namespace Flare
{
	FLARE_IMPL_TYPE(SSAO);

	SSAO::SSAO()
		: RenderPass(RenderPassQueue::PostProcessing), m_BiasPropertyIndex({}), m_RadiusPropertyIndex({}), Bias(0.1f), Radius(0.5f), BlurSize(2.0f), Enabled(true)
	{
		{
			std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SSAO");
			if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
			{
				FLARE_CORE_ERROR("SSAO: Failed to find SSAO shader");
			}
			else
			{
				m_Material = CreateRef<Material>(AssetManager::GetAsset<Shader>(shaderHandle.value()));
				m_BiasPropertyIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.Bias");
				m_RadiusPropertyIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.SampleRadius");
				m_NoiseScalePropertyIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.NoiseScale");

				m_NormalsTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_NormalsTexture");
				m_DepthTextureIndex= m_Material->GetShader()->GetPropertyIndex("u_DepthTexture");
			}

		}

		{
			std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SSAOBlur");
			if (!shaderHandle || !AssetManager::IsAssetHandleValid(shaderHandle.value()))
			{
				FLARE_CORE_ERROR("SSAO: Failed to find SSAO Blur shader");
				return;
			}
			else
			{
				m_BlurMaterial = CreateRef<Material>(AssetManager::GetAsset<Shader>(shaderHandle.value()));
				m_BlurSizePropertyIndex = m_BlurMaterial->GetShader()->GetPropertyIndex("u_Params.BlurSize");
				m_TexelSizePropertyIndex = m_BlurMaterial->GetShader()->GetPropertyIndex("u_Params.TexelSize");

				m_ColorTexture = m_BlurMaterial->GetShader()->GetPropertyIndex("u_ColorTexture");
				m_AOTexture = m_BlurMaterial->GetShader()->GetPropertyIndex("u_AOTexture");
			}
		}
	}

	void SSAO::OnRender(RenderingContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		if (!Enabled || !Renderer::GetCurrentViewport().PostProcessingEnabled)
			return;

		if (m_Material == nullptr || m_BlurMaterial == nullptr)
			return;

		Ref<FrameBuffer> aoTarget = nullptr;
		size_t targetIndex = 0;

		const Viewport& currentViewport = Renderer::GetCurrentViewport();
		if (&currentViewport == &Renderer::GetMainViewport())
			targetIndex = 0;
		else
			targetIndex = 1;

		aoTarget = m_AOTargets[targetIndex];

		if (!aoTarget)
		{
			FrameBufferSpecifications specifications = FrameBufferSpecifications(
				currentViewport.GetSize().x, currentViewport.GetSize().y,
				{ {FrameBufferTextureFormat::RF32, TextureWrap::Clamp, TextureFiltering::Closest } }
			);

			aoTarget = FrameBuffer::Create(specifications);
			m_AOTargets[targetIndex] = aoTarget;
		}
		else
		{
			const auto& specifications = aoTarget->GetSpecifications();
			if (specifications.Width != currentViewport.GetSize().x || specifications.Height != currentViewport.GetSize().y)
				aoTarget->Resize(currentViewport.GetSize().x, currentViewport.GetSize().y);
		}

		glm::vec2 texelSize = glm::vec2(1.0f / currentViewport.GetSize().x, 1.0f / currentViewport.GetSize().y);

		aoTarget->Bind();

		{
			FLARE_PROFILE_SCOPE("SSAO::MainPass");

			if (m_NormalsTextureIndex)
				m_Material->GetPropertyValue<TexturePropertyValue>(*m_NormalsTextureIndex).SetFrameBuffer(currentViewport.RenderTarget, currentViewport.NormalsAttachmentIndex);

			if (m_DepthTextureIndex)
				m_Material->GetPropertyValue<TexturePropertyValue>(*m_DepthTextureIndex).SetFrameBuffer(currentViewport.RenderTarget, currentViewport.DepthAttachmentIndex);

			if (m_BiasPropertyIndex)
				m_Material->WritePropertyValue(*m_BiasPropertyIndex, Bias);
			if (m_RadiusPropertyIndex)
				m_Material->WritePropertyValue(*m_RadiusPropertyIndex, Radius);
			if (m_NoiseScalePropertyIndex)
				m_Material->WritePropertyValue(*m_NoiseScalePropertyIndex, (glm::vec2)currentViewport.GetSize() / 8.0f);

			Renderer::DrawFullscreenQuad(m_Material);
		}

		context.RenderTarget->Bind();

		{
			FLARE_PROFILE_SCOPE("SSAO::BlurAndCombinePass");

			if (m_AOTexture)
				m_BlurMaterial->GetPropertyValue<TexturePropertyValue>(*m_AOTexture).SetFrameBuffer(aoTarget, 0);
			if (m_ColorTexture)
				m_BlurMaterial->GetPropertyValue<TexturePropertyValue>(*m_ColorTexture).SetFrameBuffer(currentViewport.RenderTarget, 0);

			if (m_BlurSizePropertyIndex)
				m_BlurMaterial->WritePropertyValue(*m_BlurSizePropertyIndex, BlurSize);
			if (m_TexelSizePropertyIndex)
				m_BlurMaterial->WritePropertyValue(*m_TexelSizePropertyIndex, texelSize);

			Renderer::DrawFullscreenQuad(m_BlurMaterial);
		}
	}
}
