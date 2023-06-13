#pragma once

#include "Flare.h"
#include "Flare/Core/Event.h"

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
}