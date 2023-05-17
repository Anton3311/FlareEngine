#pragma once

#include <Flare/Core/Core.h>

#include <string>
#include <stdint.h>

namespace Flare
{
	struct WindowProperties
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
	};

	class Window
	{
	public:
		virtual bool ShouldClose() = 0;
		virtual WindowProperties& GetProperties() = 0;

		virtual void OnUpdate() = 0;
	public:
		static Scope<Window> Create(WindowProperties& properties);
	};
}