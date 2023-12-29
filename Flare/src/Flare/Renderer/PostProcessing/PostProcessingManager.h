#pragma once

#include "Flare/Renderer/PostProcessing/ToneMapping.h"
#include "Flare/Renderer/PostProcessing/Vignette.h"
#include "Flare/Renderer/PostProcessing/SSAO.h"

namespace Flare
{
	struct PostProcessingManager
	{
		Ref<ToneMapping> ToneMappingPass;
		Ref<Vignette> VignettePass;
		Ref<SSAO> SSAOPass;
	};
}