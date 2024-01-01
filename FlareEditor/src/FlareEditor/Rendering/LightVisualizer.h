#pragma once

#include "FlareECS/World.h"
#include "FlareECS/System/System.h"
#include "FlareECS/System/SystemInitializer.h"

namespace Flare
{
	class LightVisualizer : public System
	{
	public:
		FLARE_SYSTEM;

		void OnConfig(World& world, SystemConfig& config) override;
		void OnUpdate(World& world, SystemExecutionContext& context) override;
	private:
		Query m_Query;
	};
}