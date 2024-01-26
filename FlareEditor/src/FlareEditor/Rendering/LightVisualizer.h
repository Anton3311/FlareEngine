#pragma once

#include "FlareECS/World.h"
#include "FlareECS/System/System.h"
#include "FlareECS/System/SystemInitializer.h"

#include "Flare/Renderer/Material.h"

namespace Flare
{
	class LightVisualizer : public System
	{
	public:
		FLARE_SYSTEM;

		void OnConfig(World& world, SystemConfig& config) override;
		void OnUpdate(World& world, SystemExecutionContext& context) override;
	private:
		void ReloadShaders();
	private:
		Query m_DirectionalLightQuery;
		Query m_PointLightsQuery;
		Query m_SpotlightsQuery;

		Ref<Material> m_DebugIconsMaterial;
		bool m_HasProjectOpenHandler = false;
	};
}