#pragma once

#include "Flare.h"
#include "Flare/Core/Event.h"
#include "Flare/Core/KeyCode.h"

namespace Flare
{
	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		EVENT_CLASS(WindowClose);
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