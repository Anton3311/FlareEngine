#include "InputManager.h"

#include <stdint.h>

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Core/Application.h"

#include "FlarePlatform/Events.h"

namespace Flare
{
	enum class InputState : uint8_t
	{
		None,
		Pressed,
		Released,
	};

	struct InputManagerData
	{
		glm::ivec2 PreviousMousePosition;
		glm::ivec2 MousePosition;

		glm::ivec2 MousePositionOffset;
		glm::vec2 MouseScroll;

		InputState MouseButtonState[(size_t)MouseCode::ButtonLast + 1];
		InputState KeyState[(size_t)KeyCode::Menu + 1];

		bool KeyHeld[(size_t)KeyCode::Menu + 1];
		bool MouseButtonHeld[(size_t)MouseCode::ButtonLast + 1];

		CursorMode CursorMode;
	};

	InputManagerData* s_InputData;

	void InputManager::Initialize()
	{
		s_InputData = new InputManagerData();

		s_InputData->MousePosition = glm::ivec2(0);
		s_InputData->PreviousMousePosition = glm::ivec2(0);
		s_InputData->MousePositionOffset = glm::ivec2(0);

		std::memset(s_InputData->MouseButtonState, (int32_t)InputState::None, sizeof(s_InputData->MouseButtonState));
		std::memset(s_InputData->KeyState, (int32_t)InputState::None, sizeof(s_InputData->KeyState));

		std::memset(s_InputData->KeyHeld, false, sizeof(s_InputData->KeyHeld));
		std::memset(s_InputData->MouseButtonHeld, false, sizeof(s_InputData->MouseButtonHeld));
	}

	void InputManager::Update()
	{
		FLARE_PROFILE_FUNCTION();

		s_InputData->MouseScroll = glm::vec2(0.0f);
		s_InputData->PreviousMousePosition = s_InputData->MousePosition;

		std::memset(s_InputData->MouseButtonState, (int32_t)InputState::None, sizeof(s_InputData->MouseButtonState));
		std::memset(s_InputData->KeyState, (int32_t)InputState::None, sizeof(s_InputData->KeyState));
	}

	void InputManager::ProcessEvent(Event& event)
	{
		FLARE_PROFILE_FUNCTION();
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& e) -> bool
		{
			if (!s_InputData->KeyHeld[(size_t)e.GetKeyCode()])
			{
				s_InputData->KeyState[(size_t)e.GetKeyCode()] = InputState::Pressed;
				s_InputData->KeyHeld[(size_t)e.GetKeyCode()] = true;
			}

			return false;
		});

		dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent& e) -> bool
		{
			if (s_InputData->KeyHeld[(size_t)e.GetKeyCode()])
			{
				s_InputData->KeyState[(size_t)e.GetKeyCode()] = InputState::Released;
				s_InputData->KeyHeld[(size_t)e.GetKeyCode()] = false;
			}

			return false;
		});

		dispatcher.Dispatch<MouseMoveEvent>([](MouseMoveEvent& e) -> bool
		{
			s_InputData->PreviousMousePosition = s_InputData->MousePosition;
			s_InputData->MousePosition = e.GetPosition();
			return false;
		});
		
		dispatcher.Dispatch<MouseScrollEvent>([](MouseScrollEvent& e) -> bool
		{
			s_InputData->MouseScroll = e.GetOffset();
			return false;
		});

		dispatcher.Dispatch<MouseButtonPressedEvent>([](MouseButtonPressedEvent& e) -> bool
		{
			s_InputData->MouseButtonState[(size_t)e.GetMouseCode()] = InputState::Pressed;
			s_InputData->MouseButtonHeld[(size_t)e.GetMouseCode()] = true;
			return false;
		});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([](MouseButtonReleasedEvent& e) -> bool
		{
			s_InputData->MouseButtonState[(size_t)e.GetMouseCode()] = InputState::Released;
			s_InputData->MouseButtonHeld[(size_t)e.GetMouseCode()] = false;
			return false;
		});
	}

	bool InputManager::IsKeyHeld(KeyCode key)
	{
		return s_InputData->KeyHeld[(size_t)key];
	}

	bool InputManager::IsKeyPressed(KeyCode key)
	{
		return s_InputData->KeyState[(size_t)key] == InputState::Pressed;
	}

	bool InputManager::IsKeyReleased(KeyCode key)
	{
		return s_InputData->KeyState[(size_t)key] == InputState::Released;
	}

	bool InputManager::IsMouseButtonHeld(MouseCode button)
	{
		return s_InputData->MouseButtonHeld[(size_t)button];
	}

	bool InputManager::IsMouseButtonPressed(MouseCode button)
	{
		return s_InputData->MouseButtonState[(size_t)button] == InputState::Pressed;
	}

	bool InputManager::IsMouseButtonReleased(MouseCode button)
	{
		return s_InputData->MouseButtonState[(size_t)button] == InputState::Released;
	}

	glm::vec2 InputManager::GetMouseScroll()
	{
		return s_InputData->MouseScroll;
	}

	void InputManager::SetMousePositionOffset(const glm::ivec2& offset)
	{
		s_InputData->MousePositionOffset = offset;
	}

	glm::ivec2 InputManager::GetMousePositionOffset()
	{
		return s_InputData->MousePositionOffset;
	}

	glm::ivec2 InputManager::GetMousePosition()
	{
		return s_InputData->MousePosition + s_InputData->MousePositionOffset;
	}

	glm::ivec2 InputManager::GetRawMousePosition()
	{
		return s_InputData->MousePosition;
	}

	glm::ivec2 InputManager::GetMouseDelta()
	{
		return s_InputData->MousePosition - s_InputData->PreviousMousePosition;
	}

	void InputManager::SetCursorMode(CursorMode mode)
	{
		s_InputData->CursorMode = mode;
		Application::GetInstance().GetWindow()->SetCursorMode(mode);
	}

	CursorMode InputManager::GetCursorMode()
	{
		return s_InputData->CursorMode;
	}
}
