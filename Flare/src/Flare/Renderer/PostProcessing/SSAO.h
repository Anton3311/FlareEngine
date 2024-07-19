#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/PostProcessing/PostProcessingEffect.h"

namespace Flare
{
	class Material;

	class FLARE_API SSAO : public PostProcessingEffect
	{
	public:
		FLARE_TYPE;

		SSAO();

		void RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport) override;
		const SerializableObjectDescriptor& GetSerializationDescriptor() const override;
	public:
		float Bias;
		float Radius;
		float BlurSize;
	};

	template<>
	struct TypeSerializer<SSAO>
	{
		void OnSerialize(SSAO& ssao, SerializationStream& stream)
		{
			stream.Serialize("Radius", SerializationValue(ssao.Radius));
			stream.Serialize("Bias", SerializationValue(ssao.Bias));
			stream.Serialize("BlurSize", SerializationValue(ssao.BlurSize));
		}
	};

	class FLARE_API SSAOMainPass : public RenderGraphPass
	{
	public:
		SSAOMainPass(RenderGraphTextureId normalsTexture, RenderGraphTextureId depthTexture);

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		Ref<Material> m_Material = nullptr;

		RenderGraphTextureId m_NormalsTexture;
		RenderGraphTextureId m_DepthTexture;

		Ref<SSAO> m_Parameters = nullptr;
	};

	class FLARE_API SSAOComposingPass : public RenderGraphPass
	{
	public:
		SSAOComposingPass(RenderGraphTextureId colorTexture, RenderGraphTextureId aoTexture);

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		RenderGraphTextureId m_ColorTexture;

		RenderGraphTextureId m_AOTexture;
		Ref<Material> m_Material = nullptr;

		Ref<SSAO> m_Parameters = nullptr;
	};
}
