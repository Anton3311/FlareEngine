#pragma once

#include "Flare.h"
#include "Flare/Core/Events.h"
#include "Flare/Core/WindowControls.h"

#include <string>
#include <stdint.h>

#include <glm/glm.hpp>

namespace Flare
{
	struct WindowProperties
	{
		std::string Title;
		uint32_t Width = 0;
		uint32_t Height = 0;
		bool IsMinimized = false;
		bool IsMaximized = false;
		bool CustomTitleBar = false;
		bool VSyncEnabled = true;
	};

	class Window
	{
	public:
		virtual const WindowProperties& GetProperties() const = 0;

		virtual void SetTitle(const std::string& title) = 0;
		virtual void Hide() = 0;
		virtual void SetMaximized(bool value) = 0;

		virtual void SetEventCallback(const EventCallback& callback) = 0;
		virtual void SetVSync(bool vsync) = 0;

		virtual void* GetNativeWindow() = 0;

		virtual void OnUpdate() = 0;

		virtual WindowControls& GetWindowControls() = 0;
		virtual glm::uvec2 GetControlsButtonSize() = 0;
	public:
		static Scope<Window> Create(WindowProperties& properties);
	};
}