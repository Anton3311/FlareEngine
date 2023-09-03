#pragma once

#include "FlareCore/Core.h"

#include <string_view>
#include <functional>

namespace Flare
{
	enum class EventType
	{
		WindowClose,
		WindowResize,
		WindowMove,

		MouseButtonPressed,
		MouseButtonReleased,

		MouseMove,
		MouseScroll,

		KeyPressed,
		KeyReleased,
		KeyTyped,
	};

	class FLAREPLATFORM_API Event
	{
	public:
		virtual EventType GetType() const = 0;
		virtual std::string_view GetName() const = 0;
	public:
		bool Handled = false;
	};

#define EVENT_CLASS(name) \
	public: \
	virtual EventType GetType() const override { return EventType::##name; } \
	virtual std::string_view GetName() const override { return #name; } \
	static const EventType StaticEventType = EventType::##name; \

#define FLARE_BIND_EVENT_CALLBACK(func) [this](auto&&... args) -> decltype(auto) { return this->func(std::forward<decltype(args)>(args)...); }

	using EventCallback = std::function<void(Event&)>;

	class EventDispatcher
	{
	public:
		EventDispatcher(Event& event)
			: m_Event(event) {}
	public:
		template<typename T, typename F>
		bool Dispatch(const F& function)
		{
			EventType type = m_Event.GetType();
			if (m_Event.GetType() == T::StaticEventType)
			{
				m_Event.Handled |= function((T&)m_Event);
				return true;
			}
			return false;
		}
	private:
		Event& m_Event;
	};
}