#pragma once

#include "Flare/Renderer/PostProcessing/ToneMapping.h"

namespace Flare
{
	struct PostProcessingManager
	{
		Ref<ToneMapping> ToneMappingPass;
	};
}