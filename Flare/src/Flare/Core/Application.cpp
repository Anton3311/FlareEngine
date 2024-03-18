#include "Application.h"

#include "Flare/Core/Time.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/DebugRenderer.h"
#include "Flare/Renderer/RenderCommand.h"

#include "Flare/Scripting/ScriptingEngine.h"
#include "Flare/Input/InputManager.h"

#include "FlarePlatform/Event.h"
#include "FlarePlatform/Events.h"

#include "FlarePlatform/Platform.h"
#include "FlarePlatform/Windows/WindowsWindow.h"

namespace Flare
{
	Application* s_Instance = nullptr;

	Application::Application(CommandLineArguments arguments)
		: m_Running(true), m_CommandLineArguments(arguments), m_PreviousFrameTime(0)
	{
		s_Instance = this;

		WindowProperties properties;
		properties.Title = "Flare Engine";
		properties.Size = glm::uvec2(1280, 720);
		properties.CustomTitleBar = RendererAPI::GetAPI() == RendererAPI::API::OpenGL;

		m_Window = Window::Create(properties);
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
			As<WindowsWindow>(m_Window)->SetUsesOpenGL();
			break;
		case RendererAPI::API::Vulkan:
			As<WindowsWindow>(m_Window)->SetUsesVulkan();
			break;
		}

		m_Window->Initialize();

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

			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
				return;


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
	}

	Application::~Application()
	{
		ScriptingEngine::Shutdown();

		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			Renderer2D::Shutdown();
			DebugRenderer::Shutdown();
			Renderer::Shutdown();
		}
	}

	void Application::Run()
	{
		InputManager::Initialize();

		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			RenderCommand::Initialize();
			Renderer::Initialize();
			Renderer2D::Initialize();
			DebugRenderer::Initialize();
		}

		ScriptingEngine::Initialize();

		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			RenderCommand::SetLineWidth(1.2f);

			for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
				layer->OnAttach();
		}

		while (m_Running)
		{
			Profiler::BeginFrame();
			FLARE_PROFILE_BEGIN_FRAME("Main");

			{
				FLARE_PROFILE_SCOPE("Application::Update");

				float currentTime = Platform::GetTime();
				float deltaTime = currentTime - m_PreviousFrameTime;

				Time::UpdateDeltaTime();

				InputManager::Update();
				m_Window->OnUpdate();

				if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
				{
					{
						FLARE_PROFILE_SCOPE("Layers::OnUpdate");
						for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
							layer->OnUpdate(deltaTime);
					}

					{
						FLARE_PROFILE_SCOPE("Layers::OnImGui");
						for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
							layer->OnImGUIRender();
					}
				}

				{
					FLARE_PROFILE_SCOPE("SwapBuffers");
					m_GraphicsContext->SwapBuffers();
				}

				m_PreviousFrameTime = currentTime;
			}

			FLARE_PROFILE_END_FRAME("Main");
			Profiler::EndFrame();
		}

		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
				layer->OnDetach();
		}
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