#pragma once

#include "FlarePlatform/Event.h"

#include <string>

namespace Flare
{
	class Layer
	{
	public:
		Layer(std::string_view debugName)
			: m_DebugName(debugName) {}

		virtual ~Layer() {}

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnUpdate(float deltaTime) {}
		virtual void OnImGUIRender() {}

		virtual void OnEvent(Event& event) {}
	public:
		const std::string& GetDebugName() const { return m_DebugName; }
	private:
		std::string m_DebugName;
	};
}