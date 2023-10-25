#pragma once

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Shader.h"

namespace Flare
{
	class FLARE_API ToneMapping : public RenderPass
	{
	public:
		ToneMapping();

		void OnRender(RenderingContext& context) override;
	public:
		bool Enabled;
	private:
		Ref<Shader> m_Shader;
	};
}