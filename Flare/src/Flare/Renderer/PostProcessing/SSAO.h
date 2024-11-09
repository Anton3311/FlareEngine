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

	class FLARE_API SSAO : public PostProcessingEffect
	{
	public:
		FLARE_TYPE;
		FLARE_SERIALIZABLE;

		SSAO() = default;

		void RegisterRenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld) override;
		const SerializableObjectDescriptor& GetSerializationDescriptor() const override;
	private:
		void RegisterHBAORenderPasses(RenderGraph& renderGraph, Entity viewportEntity, const World& renderWorld);
	public:
		float Bias = 30.0f;
		float Radius = 1.0f;
		float Intensity = 1.0f;
		float Sharpness = 1.0f;
	};

	template<>
	struct TypeSerializer<SSAO>
	{
		void OnSerialize(SSAO& ssao, SerializationStream& stream)
		{
			stream.Serialize("Radius", SerializationValue(ssao.Radius));
			stream.Serialize("Bias", SerializationValue(ssao.Bias));
			stream.Serialize("Intensity", SerializationValue(ssao.Intensity));
			stream.Serialize("Sharpness", SerializationValue(ssao.Sharpness));
		}
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

		Ref<SSAO> m_Parameters = nullptr;
	};
}
