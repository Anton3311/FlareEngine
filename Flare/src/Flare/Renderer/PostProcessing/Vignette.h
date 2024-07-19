#pragma once

#include "FlareCore/Serialization/Serialization.h"
#include "FlareCore/Serialization/SerializationStream.h"

#include "Flare/Renderer/PostProcessing/PostProcessingEffect.h"
#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class Material;

	class FLARE_API Vignette : public PostProcessingEffect
	{
	public:
		FLARE_TYPE;

		Vignette();

		void RegisterRenderPasses(RenderGraph& renderGraph, const Viewport& viewport) override;
		const SerializableObjectDescriptor& GetSerializationDescriptor() const override;
	public:
		glm::vec4 Color;
		float Radius;
		float Smoothness;
	};

	template<>
	struct TypeSerializer<Vignette>
	{
		FLARE_API static void OnSerialize(Vignette& vignette, SerializationStream& stream);
	};

	class FLARE_API VignettePass : public RenderGraphPass
	{
	public:
		VignettePass();
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		Ref<Vignette> m_Parameters = nullptr;
		Ref<Material> m_Material = nullptr;
	};
}