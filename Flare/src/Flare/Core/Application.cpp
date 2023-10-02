#include "Application.h"

#include "Flare/Core/Time.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/DebugRenderer.h"
#include "Flare/Renderer/RenderCommand.h"

#include "Flare/Scripting/ScriptingEngine.h"
#include "Flare/Input/InputManager.h"

#include "FlarePlatform/Event.h"
#include "FlarePlatform/Events.h"

#include "FlarePlatform/Platform.h"

namespace Flare
{
	Application* s_Instance = nullptr;

	Application::Application(CommandLineArguments arguments)
		: m_Running(true), m_CommandLineArguments(arguments)
	{
		s_Instance = this;

		WindowProperties properties;
		properties.Title = "Flare Engine";
		properties.Size = glm::uvec2(1280, 720);
		properties.CustomTitleBar = true;

		m_Window = Window::Create(properties);
		m_GraphicsContext = GraphicsContext::Create(m_Window->GetNativeWindow());
		m_GraphicsContext->Initialize();
		
		m_Window->SetEventCallback([this](Event& event)
		{
			EventDispatcher dispatcher(event);
			dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) -> bool
			{
				Close();
				return true;
			});

			dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) -> bool
			{
				RenderCommand::SetViewport(0, 0, event.GetWidth(), event.GetHeight());
				return true;
			});

			auto& layer = m_LayersStack.GetLayers();
			for (auto it = layer.end(); it != layer.begin();)
			{
				(--it)->get()->OnEvent(event);
				if (event.Handled)
					return;
			}
		});

		InputManager::Initialize();

		RenderCommand::Initialize();
		Renderer::Initialize();
		Renderer2D::Initialize();
		DebugRenderer::Initialize();

		ScriptingEngine::Initialize();

		RenderCommand::SetLineWidth(1.2f);
	}

	Application::~Application()
	{
		ScriptingEngine::Shutdown();
		Renderer::Shutdown();
		Renderer2D::Shutdown();
		DebugRenderer::Shutdown();
	}

	void Application::Run()
	{
		for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
			layer->OnAttach();

		while (m_Running)
		{
			float currentTime = Platform::GetTime();
			float deltaTime = currentTime - m_PreviousFrameTime;

			Time::UpdateDeltaTime();

			InputManager::Update();
			m_Window->OnUpdate();

			for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
				layer->OnUpdate(deltaTime);

			for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
				layer->OnImGUIRender();

			m_GraphicsContext->SwapBuffers();
			m_PreviousFrameTime = currentTime;
		}

		for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
			layer->OnDetach();
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::PushLayer(const Ref<Layer>& layer)
	{
		m_LayersStack.PushLayer(layer);
	}
	
	void Application::PushOverlay(const Ref<Layer>& layer)
	{
		m_LayersStack.PushOverlay(layer);
	}

	Application& Application::GetInstance()
	{
		FLARE_CORE_ASSERT(s_Instance != nullptr, "Application instance is not valid");
		return *s_Instance;
	}
}