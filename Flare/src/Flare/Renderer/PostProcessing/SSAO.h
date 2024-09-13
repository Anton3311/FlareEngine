#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/ShaderConstantBuffer.h"
#include "Flare/Renderer/ShaderDescriptorBuffer.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/PostProcessing/PostProcessingEffect.h"

namespace Flare
{
	class ComputeShader;
	class Material;

	enum class SSAOImplementation
	{
		SSAO,
		HBAO,
	};

	class FLARE_API SSAO : public PostProcessingEffect
	{
	public:
		FLARE_TYPE;
		FLARE_SERIALIZABLE;

		SSAO();

		void RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport) override;
		const SerializableObjectDescriptor& GetSerializationDescriptor() const override;
	private:
		void RegisterSSAORenderPasses(RenderGraph& renderGraph, const Viewport& viewport);
		void RegisterHBAORenderPasses(RenderGraph& renderGraph, const Viewport& viewport);
	public:
		float Bias;
		float Radius;
		float BlurSize;
		float Intensity = 1.0f;
		SSAOImplementation Implementation;
	};

	template<>
	struct TypeSerializer<SSAO>
	{
		void OnSerialize(SSAO& ssao, SerializationStream& stream)
		{
			stream.Serialize("Radius", SerializationValue(ssao.Radius));
			stream.Serialize("Bias", SerializationValue(ssao.Bias));
			stream.Serialize("BlurSize", SerializationValue(ssao.BlurSize));
			stream.Serialize("Intensity", SerializationValue(ssao.Intensity));

			auto implementationType = (std::underlying_type_t<SSAOImplementation>)(ssao.Implementation);
			stream.Serialize("Implementation", SerializationValue(implementationType));
			ssao.Implementation = (SSAOImplementation)implementationType;
		}
	};

	class FLARE_API SSAOMainPass : public RenderGraphPass
	{
	public:
		SSAOMainPass(RenderGraphTextureId normalsTexture, RenderGraphTextureId depthTexture, RenderGraphTextureId aoTexture);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		Ref<Material> m_Material = nullptr;

		RenderGraphTextureId m_NormalsTexture;
		RenderGraphTextureId m_DepthTexture;
		RenderGraphTextureId m_AOTexture;

		Ref<ComputeShader> m_Shader = nullptr;
		ShaderConstantBuffer m_ConstantBuffer;
		ShaderDescriptorBuffer m_DescriptorBuffer;

		std::optional<size_t> m_NormalsTextureProperty;
		std::optional<size_t> m_DepthTextureProperty;
		std::optional<size_t> m_AOImageProperty;

		std::optional<size_t> m_BiasProperty;
		std::optional<size_t> m_ImageSizeProperty;
		std::optional<size_t> m_RadiusProperty;

		Ref<SSAO> m_Parameters = nullptr;
	};

	class FLARE_API SSAOComposingPass : public RenderGraphPass
	{
	public:
		SSAOComposingPass(RenderGraphTextureId colorTexture, RenderGraphTextureId aoTexture);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		RenderGraphTextureId m_ColorTexture;
		RenderGraphTextureId m_AOTexture;

		Ref<ComputeShader> m_Shader = nullptr;
		ShaderConstantBuffer m_ConstantBuffer;
		ShaderDescriptorBuffer m_DescriptorBuffer;

		std::optional<size_t> m_ColorImageProperty;
		std::optional<size_t> m_AOImageProperty;
		std::optional<size_t> m_ImageSizeProperty;
		std::optional<size_t> m_BlurSizeProperty;

		Ref<SSAO> m_Parameters = nullptr;
	};
}
