#pragma once

#include "Flare/Core/KeyCode.h"
#include "Flare/Core/MouseCode.h"

namespace Flare::Internal
{
	struct InputBindings
	{
		using IsKeyPressedFunction = bool(*)(KeyCode key);
		IsKeyPressedFunction IsKeyPressed;

		using IsMouseButtonPressedFunction = bool(*)(MouseCode button);
		IsMouseButtonPressedFunction IsMouseButtonPressed;

		static InputBindings Bindings;
	};

	class Input
	{
	public:
		inline static bool IsKeyPressed(KeyCode key) { return InputBindings::Bindings.IsKeyPressed(key); }
		inline static bool IsMouseButtonPressed(MouseCode button) { return InputBindings::Bindings.IsMouseButtonPressed(button); }
	};
}