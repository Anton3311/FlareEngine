#pragma once

#include "FlareECS/System/SystemData.h"

namespace Flare
{
	class System
	{
	public:
		virtual void OnUpdate(SystemExecutionContext& context) = 0;
	};
}