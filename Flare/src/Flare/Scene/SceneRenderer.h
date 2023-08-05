#pragma once

#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Scene/Components.h"

#include "FlareECS/World.h"

namespace Flare
{
	struct SpritesRendererSystem : public System
	{
	public:
		SpritesRendererSystem();

		virtual void OnUpdate(SystemExecutionContext& context) override;
	private:
		Query m_SpritesQuery;
	};
}