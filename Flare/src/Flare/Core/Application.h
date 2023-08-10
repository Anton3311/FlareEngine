#pragma once

#include "Flare/Core/Core.h"
#include "Flare/Core/LayerStack.h"
#include "Flare/Core/CommandLineArguments.h"

#include "Flare/Renderer/GraphicsContext.h"

#include "FlarePlatform/Window.h"

namespace Flare
{
	class FLARE_API Application
	{
	public:
		Application(CommandLineArguments arguments);
		virtual ~Application();

		void Run();
		void Close();

		void PushLayer(const Ref<Layer>& layer);
		void PushOverlay(const Ref<Layer>& layer);

		Ref<Window> GetWindow() const { return m_Window; }
		CommandLineArguments GetCommandLineArguments() const { return m_CommandLineArguments; }

		static Application& GetInstance();
	protected:
		Ref<Window> m_Window;
		CommandLineArguments m_CommandLineArguments;
	private:
		Scope<GraphicsContext> m_GraphicsContext;
		LayerStack m_LayersStack;

		bool m_Running;
		float m_PreviousFrameTime;
	};
}