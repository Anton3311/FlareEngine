#pragma once

#include "FlareCore/Core.h"
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

		void ExecuteAfterEndOfFrame(std::function<void()>&& function);

		Ref<Window> GetWindow() const { return m_Window; }
		CommandLineArguments GetCommandLineArguments() const { return m_CommandLineArguments; }

		static Application& GetInstance();
	protected:
		Ref<Window> m_Window;
		CommandLineArguments m_CommandLineArguments;
	private:
		LayerStack m_LayersStack;

		std::vector<std::function<void()>> m_AfterEndOfFrameFunctions;

		bool m_Running;
		float m_PreviousFrameTime;
	};
}