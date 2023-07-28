#pragma once

#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Scene/Components.h"

#include "FlareECS/World.h"

namespace Flare
{
	struct SpritesRendererSystem
	{
		static Query Setup(World& world);
		static void OnUpdate(SystemExecutionContext& context);
	};
}