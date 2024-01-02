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
		: m_BiasPropertyIndex({}), m_RadiusPropertyIndex({}), Bias(0.1f), Radius(0.5f), BlurSize(2.0f), Enabled(true)
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
			}
		}
	}

	void SSAO::OnRender(RenderingContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		if (!Enabled)
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
			currentViewport.RenderTarget->BindAttachmentTexture(1, 0);

			if (&currentViewport == &Renderer::GetMainViewport())
				currentViewport.RenderTarget->BindAttachmentTexture(2, 1);
			else
				currentViewport.RenderTarget->BindAttachmentTexture(3, 1);

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
			aoTarget->BindAttachmentTexture(0);
			context.RenderTarget->BindAttachmentTexture(0, 1);

			if (m_BlurSizePropertyIndex)
				m_BlurMaterial->WritePropertyValue(*m_BlurSizePropertyIndex, BlurSize);
			if (m_TexelSizePropertyIndex)
				m_BlurMaterial->WritePropertyValue(*m_TexelSizePropertyIndex, texelSize);

			Renderer::DrawFullscreenQuad(m_BlurMaterial);
		}
	}
}
