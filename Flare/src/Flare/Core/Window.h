#pragma once

#include "Flare.h"
#include "Flare/Core/Events.h"

#include <string>
#include <stdint.h>

namespace Flare
{
	struct WindowProperties
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		bool IsMinimized;
		bool VSyncEnabled;
	};

	class Window
	{
	public:
		virtual const WindowProperties& GetProperties() const = 0;

		virtual void SetEventCallback(const EventCallback& callback) = 0;
		virtual void SetVSync(bool vsync) = 0;

		virtual void OnUpdate() = 0;
	public:
		static Scope<Window> Create(WindowProperties& properties);
	};
}