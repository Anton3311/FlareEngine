#include "InputManager.h"

#include "FlarePlatform/Events.h"

namespace Flare
{
	InputManager::Data s_Data;

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

	bool InputManager::IsKeyPressed(KeyCode key)
	{
		return s_Data.KeyState[(size_t)key];
	}
	
	bool InputManager::IsMouseButtonPreseed(MouseCode button)
	{
		return s_Data.MouseButtonsState[(size_t)button];
	}

	void InputManager::SetMousePositionOffset(const glm::ivec2& offset)
	{
		s_Data.MousePositionOffset = offset;
	}

	glm::ivec2 InputManager::GetMousePositionOffset()
	{
		return s_Data.MousePositionOffset;
	}

	glm::ivec2 InputManager::GetMousePosition()
	{
		return s_Data.MousePosition + s_Data.MousePositionOffset;
	}

	glm::ivec2 InputManager::GetRawMousePosition()
	{
		return s_Data.MousePosition;
	}

	glm::ivec2 InputManager::GetMouseDelta()
	{
		return s_Data.MousePosition - s_Data.PreviousMousePosition;
	}
}
