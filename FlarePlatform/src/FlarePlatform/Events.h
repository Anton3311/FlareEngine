#pragma once

#include "FlareCore/KeyCode.h"
#include "FlareCore/MouseCode.h"

#include "FlarePlatform/Event.h"

#include <glm/glm.hpp>

namespace Flare
{
	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS(WindowClose);
	};

	class WindowFocusEvent : public Event
	{
	public:
		WindowFocusEvent(bool focused)
			: m_Focused(focused) {}

		constexpr bool IsFocused() const { return m_Focused; }

		EVENT_CLASS(WindowFocus);
	public:
	private:
		bool m_Focused;
	};

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(uint32_t width, uint32_t height)
			: m_Width(width), m_Height(height) {}

		inline uint32_t GetWidth() const { return m_Width; }
		inline uint32_t GetHeight() const { return m_Height; }

		EVENT_CLASS(WindowResize);
	private:
		uint32_t m_Width;
		uint32_t m_Height;
	};

	class MouseButtonPressedEvent : public Event
	{
	public:
		MouseButtonPressedEvent(MouseCode mouseCode)
			: m_MouseCode(mouseCode) {}
		
		inline MouseCode GetMouseCode() const { return m_MouseCode; }

		EVENT_CLASS(MouseButtonPressed);
	private:
		MouseCode m_MouseCode;
	};

	class MouseButtonReleasedEvent : public Event
	{
	public:
		MouseButtonReleasedEvent(MouseCode mouseCode)
			: m_MouseCode(mouseCode) {}

		inline MouseCode GetMouseCode() const { return m_MouseCode; }

		EVENT_CLASS(MouseButtonReleased);
	private:
		MouseCode m_MouseCode;
	};

	class MouseScrollEvent : public Event
	{
	public:
		MouseScrollEvent(glm::vec2 offset)
			: m_Offset(offset) {}
		
		inline glm::vec2 GetOffset() const { return m_Offset; }

		EVENT_CLASS(MouseScroll);
	private:
		glm::vec2 m_Offset;
	};

	class MouseMoveEvent : public Event
	{
	public:
		MouseMoveEvent(glm::vec2 offset)
			: m_Position(offset) {}

		inline glm::vec2 GetPosition() const { return m_Position; }

		EVENT_CLASS(MouseMove);
	private:
		glm::vec2 m_Position;
	};

	class KeyPressedEvent : public Event
	{
	public:
		KeyPressedEvent(KeyCode keyCode, bool isRepeat = false)
			: m_KeyCode(keyCode), m_IsRepeat(isRepeat) {}

		inline KeyCode GetKeyCode() const { return m_KeyCode; }
		inline bool IsRepeat() const { return m_IsRepeat; }

		EVENT_CLASS(KeyPressed);
	private:
		KeyCode m_KeyCode;
		bool m_IsRepeat;
	};

	class KeyReleasedEvent : public Event
	{
	public:
		KeyReleasedEvent(KeyCode keyCode)
			: m_KeyCode(keyCode) {}

		inline KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS(KeyReleased);
	private:
		KeyCode m_KeyCode;
	};

	class KeyTypedEvent : public Event
	{
	public:
		KeyTypedEvent(KeyCode keyCode)
			: m_KeyCode(keyCode) {}

		inline KeyCode GetKeyCode() const { return m_KeyCode; }

		EVENT_CLASS(KeyTyped);
	private:
		KeyCode m_KeyCode;
	};
}