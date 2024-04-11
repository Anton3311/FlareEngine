#include "Application.h"

#include "Flare/Core/Time.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererPrimitives.h"
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
		properties.CustomTitleBar = true;

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

		GraphicsContext::Create(m_Window->GetNativeWindow());
		
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

			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
				return;

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

		Renderer2D::Shutdown();
		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			DebugRenderer::Shutdown();
		}

		Renderer::Shutdown();
		RendererPrimitives::Clear();
		GraphicsContext::Shutdown();
	}

	void Application::Run()
	{
		InputManager::Initialize();

		RenderCommand::Initialize();
		Renderer::Initialize();

		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			DebugRenderer::Initialize();
		}

		Renderer2D::Initialize();

		ScriptingEngine::Initialize();
		RenderCommand::SetLineWidth(1.2f);

		for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
			layer->OnAttach();

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

				GraphicsContext::GetInstance().BeginFrame();
				Renderer2D::BeginFrame();

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

				Renderer2D::EndFrame();

				{
					FLARE_PROFILE_SCOPE("Present");
					GraphicsContext::GetInstance().Present();
				}

				m_PreviousFrameTime = currentTime;
			}

			FLARE_PROFILE_END_FRAME("Main");
			Profiler::EndFrame();
		}

		GraphicsContext::GetInstance().WaitForDevice();

		for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
			layer->OnDetach();

		AssetManager::Uninitialize();
		Font::SetDefault(nullptr);

		m_LayersStack.Clear();
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