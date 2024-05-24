#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore//Serialization/SerializationStream.h"

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Material.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class FLARE_API ToneMapping : public RenderPass
	{
	public:
		FLARE_TYPE;

		ToneMapping();

		void OnRender(RenderingContext& context) override;
	public:
		bool Enabled;
	private:
		std::optional<uint32_t> m_ColorTexture;
		Ref<Material> m_Material;
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