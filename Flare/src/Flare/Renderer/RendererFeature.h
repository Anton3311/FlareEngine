#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	class FLARE_API RendererFeature
	{
	public:
		virtual ~RendererFeature() = default;

		virtual void OnUpdate() = 0;
		virtual void AddPasses() = 0;
	};
}
