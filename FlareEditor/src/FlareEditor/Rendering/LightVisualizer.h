#pragma once

#include "FlareECS/System/System.h"
#include "FlareECS/System/SystemInitializer.h"

namespace Flare
{
	class LightVisualizer : public System
	{
	public:
		FLARE_SYSTEM;

		void OnConfig(SystemConfig& config) override;
		void OnUpdate(SystemExecutionContext& context) override;
	private:
		Query m_Query;
	};
}