#pragma once

#include "FlareECS/System/System.h"
#include "FlareECS/System/SystemInitializer.h"

namespace Flare
{
	class CameraFrustumRenderer : public System
	{
	public:
		FLARE_SYSTEM;

		virtual void OnConfig(SystemConfig& config) override;
		virtual void OnUpdate(SystemExecutionContext& context) override;
	private:
		Query m_Query;
	};
}