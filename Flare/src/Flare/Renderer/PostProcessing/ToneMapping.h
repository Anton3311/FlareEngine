#pragma once

#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/Material.h"

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
		Ref<Material> m_Material;
	};
}