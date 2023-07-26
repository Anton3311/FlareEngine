#pragma once

#include "Flare/Core/Window.h"
#include "Flare/Core/LayerStack.h"
#include "Flare/ImGui/ImGUILayer.h"

namespace Flare
{
	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		void Close();

		void PushLayer(const Ref<Layer>& layer);
		void PushOverlay(const Ref<Layer>& layer);

		Ref<Window> GetWindow() const { return m_Window; }

		static Application& GetInstance();
	protected:
		Ref<Window> m_Window;
	private:
		LayerStack m_LayersStack;
		Ref<ImGUILayer> m_ImGuiLayer;

		bool m_Running;
		float m_PreviousFrameTime;
	private:
		static Application* s_Instance;
	};
}