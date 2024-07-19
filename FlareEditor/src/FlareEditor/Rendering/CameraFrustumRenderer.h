#pragma once

#include "FlareECS/World.h"
#include "FlareECS/System/System.h"
#include "FlareECS/System/SystemInitializer.h"

namespace Flare
{
	class CameraFrustumRenderer : public System
	{
	public:
		FLARE_SYSTEM;

		virtual void OnConfig(World& world, SystemConfig& config) override;
		virtual void OnUpdate(World& world, SystemExecutionContext& context) override;
	private:
		Query m_Query;
	};
}