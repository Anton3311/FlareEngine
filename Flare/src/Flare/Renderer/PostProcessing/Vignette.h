#pragma once

#include "FlareCore/Serialization/Serialization.h"
#include "FlareCore/Serialization/SerializationStream.h"

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/Material.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class FLARE_API Vignette : public RenderPass
	{
	public:
		FLARE_TYPE;

		Vignette();

		void OnRender(RenderingContext& context) override;
	public:
		bool Enabled;
		glm::vec4 Color;
		float Radius;
		float Smoothness;
	private:
		Ref<Material> m_Material;
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
		void OnRender(Ref<CommandBuffer> commandBuffer) override;
	private:
		Ref<Material> m_Material;
		glm::vec4 m_Color = glm::vec4(1.0f);
		float m_Radius = 1.0f;
		float m_Smoothness = 1.0f;
	};
}