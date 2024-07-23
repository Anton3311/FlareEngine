#include "Application.h"

#include "Flare/Core/Time.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Font.h"
#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/DebugRenderer/DebugRenderer.h"

#include "Flare/Scripting/ScriptingEngine.h"
#include "Flare/Input/InputManager.h"

#include "FlarePlatform/Event.h"
#include "FlarePlatform/Events.h"

#include "FlarePlatform/Platform.h"
#include "FlarePlatform/Windows/WindowsWindow.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"

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

		const std::string_view apiArgument = "--api=";
		RendererAPI::API rendererApi = RendererAPI::API::Vulkan;
		for (uint32_t i = 0; i < m_CommandLineArguments.ArgumentsCount; i++)
		{
			std::string_view argument = m_CommandLineArguments.Arguments[i];

			if (argument._Starts_with(apiArgument))
			{
				std::string_view apiName = argument.substr(apiArgument.size());

				if (apiName == "vulkan")
				{
					rendererApi = RendererAPI::API::Vulkan;
				}
			}
		}

		RendererAPI::Create(rendererApi);

		m_Window = Window::Create(properties);
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			As<WindowsWindow>(m_Window)->SetUsesVulkan();
			break;
		}

		m_Window->Initialize();

		GraphicsContext::Create(m_Window);
		
		m_Window->SetEventCallback([this](Event& event)
		{
			EventDispatcher dispatcher(event);
			dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event) -> bool
			{
				Close();
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

		Renderer2D::Shutdown();
		DebugRenderer::Shutdown();
		Renderer::Shutdown();
		RendererPrimitives::Clear();
		GraphicsContext::Shutdown();
	}

	void Application::Run()
	{
		InputManager::Initialize();

		Renderer::Initialize();
		DebugRenderer::Initialize();
		Renderer2D::Initialize();

		ScriptingEngine::Initialize();

		for (const Ref<Layer>& layer : m_LayersStack.GetLayers())
			layer->OnAttach();

		while (m_Running)
		{
			FLARE_PROFILE_BEGIN_FRAME("Main");

			{
				FLARE_PROFILE_SCOPE("Application::Update");

				float currentTime = Platform::GetTime();
				float deltaTime = currentTime - m_PreviousFrameTime;

				Time::UpdateDeltaTime();

				InputManager::Update();
				m_Window->OnUpdate();

				if (!m_Window->GetProperties().IsMinimized)
				{
					GraphicsContext::GetInstance().BeginFrame();
					Renderer::BeginFrame();
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
					Renderer::EndFrame();
				}

				{
					FLARE_PROFILE_SCOPE("Present");
					GraphicsContext::GetInstance().Present();
				}

				{
					FLARE_PROFILE_SCOPE("ExecuteAfterEndFrameFunctions");
					for (const auto& function : m_AfterEndOfFrameFunctions)
					{
						function();
					}

					m_AfterEndOfFrameFunctions.clear();
				}

				m_PreviousFrameTime = currentTime;
			}

			FLARE_PROFILE_END_FRAME("Main");
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

	void Application::ExecuteAfterEndOfFrame(std::function<void()>&& function)
	{
		m_AfterEndOfFrameFunctions.push_back(function);
	}

	Application& Application::GetInstance()
	{
		FLARE_CORE_ASSERT(s_Instance != nullptr, "Application instance is not valid");
		return *s_Instance;
	}
}