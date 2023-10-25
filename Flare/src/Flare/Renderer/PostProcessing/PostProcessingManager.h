#pragma once

#include "Flare/Renderer/PostProcessing/ToneMapping.h"
#include "Flare/Renderer/PostProcessing/Vignette.h"

namespace Flare
{
	struct PostProcessingManager
	{
		Ref<ToneMapping> ToneMappingPass;
		Ref<Vignette> VignettePass;
	};
}