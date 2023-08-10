#pragma once

#include "Flare/Core/Core.h"

namespace Flare
{
	class FLAREPLATFORM_API WindowControls
	{
	public:
		virtual bool IsTitleBarHovered() const = 0;
	};
}