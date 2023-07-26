#include "InputManager.h"

#include "Flare/Core/Events.h"

namespace Flare
{
	InputManager::Data InputManager::s_Data;

	void InputManager::Initialize()
	{
		s_Data.MousePosition = glm::ivec2(0);
		s_Data.PreviousMousePosition = glm::ivec2(0);
		s_Data.MousePositionOffset = glm::ivec2(0);

		std::memset(s_Data.MouseButtonsState, 0, sizeof(s_Data.MouseButtonsState));
		std::memset(s_Data.KeyState, 0, sizeof(s_Data.KeyState));
	}

	void InputManager::ProcessEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		Data& data = s_Data;
		dispatcher.Dispatch<KeyPressedEvent>([&data](KeyPressedEvent& e) -> bool
		{
			data.KeyState[(size_t)e.GetKeyCode()] = true;
			return false;
		});

		dispatcher.Dispatch<KeyReleasedEvent>([&data](KeyReleasedEvent& e) -> bool
		{
			data.KeyState[(size_t)e.GetKeyCode()] = false;
			return false;
		});

		dispatcher.Dispatch<MouseMoveEvent>([&data](MouseMoveEvent& e) -> bool
		{
			data.PreviousMousePosition = data.MousePosition;
			data.MousePosition = e.GetPosition();
			return false;
		});

		dispatcher.Dispatch<MouseButtonPressedEvent>([&data](MouseButtonPressedEvent& e) -> bool
		{
			data.MouseButtonsState[(size_t)e.GetMouseCode()] = true;
			return false;
		});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([&data](MouseButtonReleasedEvent& e) -> bool
		{
			data.MouseButtonsState[(size_t)e.GetMouseCode()] = false;
			return false;
		});
	}
}
