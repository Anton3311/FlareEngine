#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Viewport.h"

#include <glm/glm.hpp>

namespace Flare
{
	class FLARE_API Renderer
	{
	public:
		static void Initialize();
		static void Shutdown();

		static void SetMainViewport(Viewport& viewport);
		static void SetCurrentViewport(Viewport& viewport);

		static Viewport& GetMainViewport();
		static Viewport& GetCurrentViewport();
	};
}