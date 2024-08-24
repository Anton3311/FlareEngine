#pragma once

#include "FlareCore/Core.h"

namespace Flare
{
	class FLAREPLATFORM_API WindowControls : public RefCounted<WindowControls>
	{
	public:
		virtual bool IsTitleBarHovered() const = 0;
	};
}