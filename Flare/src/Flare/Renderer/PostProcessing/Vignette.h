#pragma once

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Shader.h"

namespace Flare
{
	class FLARE_API Vignette : public RenderPass
	{
	public:
		Vignette();

		void OnRender(RenderingContext& context) override;
	public:
		bool Enabled;
		glm::vec4 Color;
		float Radius;
		float Smoothness;
	private:
		Ref<Shader> m_Shader;
	};
}