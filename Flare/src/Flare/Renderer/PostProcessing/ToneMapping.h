#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Material.h"

#include "Flare/Renderer/PostProcessing/PostProcessingEffect.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class FLARE_API ToneMapping : public PostProcessingEffect
	{
	public:
		FLARE_TYPE;

		ToneMapping();

		void RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport) override;
		const SerializableObjectDescriptor& GetSerializationDescriptor() const override;
	public:
		bool Enabled;
	};

	template<>
	struct TypeSerializer<ToneMapping>
	{
		void OnSerialize(ToneMapping& toneMapping, SerializationStream& stream)
		{
			stream.Serialize("Enabled", SerializationValue(toneMapping.Enabled));
		}
	};



	class FLARE_API ToneMappingPass : public RenderGraphPass
	{
	public:
		ToneMappingPass(Ref<Texture> colorTexture);

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		Ref<Material> m_Material = nullptr;
		Ref<Texture> m_ColorTexture = nullptr;
	};
}