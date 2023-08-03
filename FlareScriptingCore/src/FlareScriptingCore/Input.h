#pragma once

#include "Flare/Core/KeyCode.h"
#include "Flare/Core/MouseCode.h"

#include "FlareScriptingCore/Bindings.h"

namespace Flare::Scripting
{
	class Input
	{
	public:
		inline static bool IsKeyPressed(KeyCode key)
		{
			return Bindings::Instance->IsKeyPressed(key);
		}

		inline static bool IsMouseButtonPressed(MouseCode button)
		{
			return Bindings::Instance->IsMouseButtonPressed(button);
		}
	};
}