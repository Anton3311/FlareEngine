#include "SSAO.h"

#include "Flare/Scene/Scene.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/ComputeShader.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Material.h"

#include "Flare/Renderer/Passes/BlitPass.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "FlareCore/Profiler/Profiler.h"

#include <random>

namespace Flare
{
	FLARE_IMPL_TYPE(SSAO);
	FLARE_SERIALIZABLE_IMPL(SSAO);

	SSAO::SSAO()
		: Bias(0.1f), Radius(0.5f), BlurSize(2.0f)
	{
	}

	void SSAO::RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		if (!IsEnabled())
			return;

		RenderGraphTextureId aoTexture = renderGraph.CreateTexture(TextureFormat::RF32, "SSAO.AOTexture", 0.5f);

		RenderGraphPassSpecifications ssaoMainPass{};
		ssaoMainPass.SetDebugName("SSAOMainPass");
		ssaoMainPass.AddInput(viewport.NormalsTextureId);
		ssaoMainPass.AddInput(viewport.DepthTextureId);
		ssaoMainPass.AddOutput(aoTexture, 0);

		RenderGraphPassSpecifications ssaoComposingPass{};
		ssaoComposingPass.SetDebugName("SSAOComposingPass");
		ssaoComposingPass.SetType(RenderGraphPassType::Compute);
		ssaoComposingPass.AddInput(aoTexture);
		ssaoComposingPass.AddResource(viewport.ColorTextureId, ResourceAccess::ReadWrite);

		renderGraph.AddPass(ssaoMainPass, CreateRef<SSAOMainPass>(viewport.NormalsTextureId, viewport.DepthTextureId));
		renderGraph.AddPass(ssaoComposingPass, CreateRef<SSAOComposingPass>(viewport.ColorTextureId, aoTexture));
	}

	const SerializableObjectDescriptor& SSAO::GetSerializationDescriptor() const
	{
		return FLARE_SERIALIZATION_DESCRIPTOR_OF(SSAO);
	}



	SSAOMainPass::SSAOMainPass(RenderGraphTextureId normalsTexture, RenderGraphTextureId depthTexture)
		: m_NormalsTexture(normalsTexture), m_DepthTexture(depthTexture)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SSAO");
		if (shaderHandle && AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			m_Material = Material::Create(AssetManager::GetAsset<Shader>(shaderHandle.value()));
		}
		else
		{
			FLARE_CORE_ERROR("SSAO: Failed to find SSAO shader");
		}

		auto result = Scene::GetActive()->GetPostProcessingManager().GetEffect<SSAO>();
		FLARE_CORE_ASSERT(result.has_value());
		m_Parameters = *result;
	}

	void SSAOMainPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
	}

	void SSAOMainPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		commandBuffer->SetGlobalDescriptorSet(context.GetViewport().GetFrameResources().CameraDescriptorSet, 0);

		auto biasIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.Bias");
		auto radiusIndex = m_Material->GetShader()->GetPropertyIndex("u_Params.SampleRadius");

		auto normalsTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_NormalsTexture");
		auto depthTextureIndex = m_Material->GetShader()->GetPropertyIndex("u_DepthTexture");

		m_Material->WritePropertyValue(*biasIndex, m_Parameters->Bias);
		m_Material->WritePropertyValue(*radiusIndex, m_Parameters->Radius);
		m_Material->SetTextureProperty(*normalsTextureIndex, context.GetRenderGraphResourceManager().GetTexture(m_NormalsTexture));
		m_Material->SetTextureProperty(*depthTextureIndex, context.GetRenderGraphResourceManager().GetTexture(m_DepthTexture));

		commandBuffer->SetDefaltViewportAndScissors();

		commandBuffer->ApplyMaterial(m_Material);
		commandBuffer->DrawMeshIndexed(RendererPrimitives::GetFullscreenQuadMesh(), 0, 0, 1);
	}



	SSAOComposingPass::SSAOComposingPass(RenderGraphTextureId colorTexture, RenderGraphTextureId aoTexture)
		: m_ColorTexture(colorTexture), m_AOTexture(aoTexture)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SSAOCompose");
		if (shaderHandle && AssetManager::IsAssetHandleValid(shaderHandle.value()))
		{
			m_Shader = AssetManager::GetAsset<ComputeShader>(*shaderHandle);

			Ref<const ComputeShaderMetadata> metadata = m_Shader->GetMetadata();

			m_ColorImageProperty = metadata->FindDescriptorProperty("u_Color");
			m_AOImageProperty = metadata->FindDescriptorProperty("u_AO");
			m_ImageSizeProperty = metadata->FindConstantProperty("u_ImageSize");
			m_BlurSizeProperty = metadata->FindConstantProperty("u_BlurSize");

			m_ConstantBuffer.SetShader(m_Shader);
			m_DescriptorBuffer.SetShader(m_Shader);
		}
		else
		{
			FLARE_CORE_ERROR("SSAO: Failed to find SSAOCompose shader");
		}

		auto result = Scene::GetActive()->GetPostProcessingManager().GetEffect<SSAO>();
		FLARE_CORE_ASSERT(result.has_value());
		m_Parameters = *result;
	}

	void SSAOComposingPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const TextureSpecifications& colorTextureSpec = context.GetRenderGraphResourceManager().GetTexture(m_ColorTexture)->GetSpecifications();
		m_ConstantBuffer.SetProperty(*m_ImageSizeProperty, glm::ivec2((int32_t)colorTextureSpec.Width, (int32_t)colorTextureSpec.Height));
		m_ConstantBuffer.SetProperty(*m_BlurSizeProperty, m_Parameters->BlurSize);

		m_DescriptorBuffer.SetTexture(*m_AOImageProperty, context.GetRenderGraphResourceManager().GetTexture(m_AOTexture));
		m_DescriptorBuffer.SetTexture(*m_ColorImageProperty, context.GetRenderGraphResourceManager().GetTexture(m_ColorTexture));
	}

	void SSAOComposingPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		commandBuffer->BindComputeShader(m_Shader);
		commandBuffer->PushConstants(m_ConstantBuffer);
		commandBuffer->PushDescriptorProperties(m_DescriptorBuffer);

		glm::uvec2 renderAreaSize = (glm::uvec2)context.GetViewport().GetSize();
		glm::uvec2 localGroupSize = m_Shader->GetMetadata()->LocalGroupSize;

		glm::uvec2 groupCount = (renderAreaSize + localGroupSize - glm::uvec2(1)) / localGroupSize;

		commandBuffer->DispatchCompute(glm::uvec3(groupCount, 1));
	}
}
