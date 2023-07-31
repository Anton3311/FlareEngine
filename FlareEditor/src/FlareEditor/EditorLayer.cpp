#include "EditorLayer.h"

#include "Flare.h"
#include "Flare/Core/Application.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Scene/SceneSerializer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Project/Project.h"
#include "Flare/Platform/Platform.h"

#include "Flare/Scripting/ScriptingEngine.h"
#include "Flare/Input/InputManager.h"

#include "FlareEditor/EditorContext.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include "FlareEditor/UI/SceneViewportWindow.h"
#include "FlareEditor/UI/EditorTitleBar.h"
#include "FlareEditor/UI/SystemsInspectorWindow.h"
#include "FlareEditor/UI/ProjectSettingsWindow.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Flare
{
	EditorLayer* EditorLayer::s_Instance = nullptr;

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
		s_Instance = this;
	}

	EditorLayer::~EditorLayer()
	{
		s_Instance = nullptr;
	}

	void EditorLayer::OnAttach()
	{
		Project::OnProjectOpen.Bind(FLARE_BIND_EVENT_CALLBACK(OnOpenProject));
		Project::OnUnloadActiveProject.Bind([this]()
		{
			Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());

			assetManager->UnloadAsset(EditorContext::GetActiveScene()->Handle);
			EditorContext::Uninitialize();
		});

		AssetManager::Intialize(CreateRef<EditorAssetManager>());

		m_AssetManagerWindow.SetOpenAction(AssetType::Scene, [this](AssetHandle handle)
		{
			EditorContext::OpenScene(handle);
		});

		EditorContext::Initialize();

		m_Viewports.emplace_back(CreateRef<SceneViewportWindow>(m_Camera));
		m_Viewports.emplace_back(CreateRef<ViewportWindow>("Game"));

		EditorCameraSettings& settings = m_Camera.GetSettings();
		settings.FOV = 60.0f;
		settings.Near = 0.1f;
		settings.Far = 1000.0f;
		settings.RotationSpeed = 1.0f;
		settings.DragSpeed = 0.1f;

		if (Application::GetInstance().GetCommandLineArguments().ArgumentsCount >= 2)
		{
			std::filesystem::path projectPath = Application::GetInstance().GetCommandLineArguments()[1];
			Project::OpenProject(projectPath);
		}
		else
		{
			std::optional<std::filesystem::path> projectPath = Platform::ShowOpenFileDialog(L"Flare Project (*.flareproj)\0*.flareproj\0");

			if (projectPath.has_value())
				Project::OpenProject(projectPath.value());
		}
	}

	void EditorLayer::OnDetach()
	{
		if (EditorContext::Instance.Mode == EditorMode::Play)
			ExitPlayMode();

		if (AssetManager::IsAssetHandleValid(EditorContext::GetEditedScene()->Handle))
			As<EditorAssetManager>(AssetManager::GetInstance())->UnloadAsset(EditorContext::GetEditedScene()->Handle);

		EditorContext::Uninitialize();
	}

	void EditorLayer::OnUpdate(float deltaTime)
	{
		m_PreviousFrameTime = deltaTime;

		Renderer2D::ResetStats();

		ScriptingEngine::OnFrameStart(deltaTime);

		if (!m_PlaymodePaused)
			EditorContext::GetActiveScene()->OnUpdateRuntime();

		for (auto& viewport : m_Viewports)
			viewport->OnRenderViewport();
	}

	void EditorLayer::OnEvent(Event& event)
	{
		m_Camera.ProcessEvents(event);
		InputManager::ProcessEvent(event);

		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent& e) -> bool
		{
			switch (e.GetKeyCode())
			{
			case KeyCode::G:
				EditorContext::Instance.Gizmo = GizmoMode::Translate;
				break;
			case KeyCode::R:
				EditorContext::Instance.Gizmo = GizmoMode::Rotate;
				break;
			case KeyCode::S:
				EditorContext::Instance.Gizmo = GizmoMode::Scale;
				break;
			}

			return false;
		});
	}

	void EditorLayer::OnImGUIRender()
	{
		static bool fullscreen = true;
		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;
		if (fullscreen)
		{
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspaceFlags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
			windowFlags |= ImGuiWindowFlags_NoBackground;

		static bool open = true;
		ImGui::Begin("DockSpace", &open, windowFlags);

		ImGuiID dockspaceId = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);

		EditorTitleBar titleBar;
		titleBar.OnRenderImGui();

		{
			ImGui::Begin("Renderer");

			ImGui::Text("Frame time %f", m_PreviousFrameTime);
			ImGui::Text("FPS %f", 1.0f / m_PreviousFrameTime);

			if (ImGui::ColorEdit3("Clear Color", glm::value_ptr(m_ClearColor), ImGuiColorEditFlags_Float))
				RenderCommand::SetClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 1.0);

			if (ImGui::CollapsingHeader("Renderer 2D"))
			{
				const auto& stats = Renderer2D::GetStats();
				ImGui::Text("Quads %d", stats.QuadsCount);
				ImGui::Text("Draw Calls %d", stats.DrawCalls);
				ImGui::Text("Vertices %d", stats.GetTotalVertexCount());
			}

			ImGui::End();
		}

		{
			ImGui::Begin("Settings");

			Ref<Window> window = Application::GetInstance().GetWindow();

			bool vsync = window->GetProperties().VSyncEnabled;
			if (ImGui::Checkbox("VSync", &vsync))
				window->SetVSync(vsync);

			ImGui::End();
		}

		for (auto& viewport : m_Viewports)
			viewport->OnRenderImGui();

		SystemsInspectorWindow::OnImGuiRender();
		ProjectSettingsWindow::OnRenderImGui();

		m_SceneWindow.OnImGuiRender();
		m_PropertiesWindow.OnImGuiRender();
		m_AssetManagerWindow.OnImGuiRender();

		ImGui::End();
	}

	void EditorLayer::UpdateWindowTitle()
	{
		if (Project::GetActive() != nullptr)
		{
			std::string name = fmt::format("Flare Editor - {0} - {1}", Project::GetActive()->Name, Project::GetActive()->Location.generic_string());
			Application::GetInstance().GetWindow()->SetTitle(name);
		}
	}

	void EditorLayer::OnOpenProject()
	{
		Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());

		assetManager->Reinitialize();
		EditorContext::Initialize();

		UpdateWindowTitle();
		m_AssetManagerWindow.RebuildAssetTree();

		AssetHandle startScene = Project::GetActive()->StartScene;
		EditorContext::OpenScene(startScene);
	}

	void EditorLayer::SaveActiveScene()
	{
		if (AssetManager::IsAssetHandleValid(EditorContext::GetActiveScene()->Handle))
			SceneSerializer::Serialize(EditorContext::GetActiveScene());
		else
			SaveActiveSceneAs();
	}

	void EditorLayer::SaveActiveSceneAs()
	{
		std::optional<std::filesystem::path> scenePath = Platform::ShowSaveFileDialog(L"Flare Scene (*.flare)\0*.flare\0");
		if (scenePath.has_value())
		{
			std::filesystem::path& path = scenePath.value();
			if (!path.has_extension())
				path.replace_extension(".flare");

			SceneSerializer::Serialize(EditorContext::GetActiveScene(), path);
			AssetHandle handle = As<EditorAssetManager>(AssetManager::GetInstance())->ImportAsset(path);
			EditorContext::OpenScene(handle);
		}
	}

	void EditorLayer::EnterPlayMode()
	{
		FLARE_CORE_ASSERT(EditorContext::Instance.Mode == EditorMode::Edit);

		SaveActiveScene();

		m_PlaymodePaused = false;

		Ref<Scene> playModeScene = CreateRef<Scene>(false);
		ScriptingEngine::SetCurrentECSWorld(playModeScene->GetECSWorld());

		playModeScene->CopyFrom(EditorContext::GetActiveScene());
		playModeScene->Initialize();
		playModeScene->InitializeRuntime();

		EditorContext::SetActiveScene(playModeScene);
		EditorContext::Instance.Mode = EditorMode::Play;

		EditorContext::GetActiveScene()->OnRuntimeStart();
	}

	void EditorLayer::ExitPlayMode()
	{
		EditorContext::GetActiveScene()->OnRuntimeEnd();

		FLARE_CORE_ASSERT(EditorContext::Instance.Mode == EditorMode::Play);

		EditorContext::SetActiveScene(EditorContext::GetEditedScene());

		EditorContext::Instance.Mode = EditorMode::Edit;

		ScriptingEngine::SetCurrentECSWorld(EditorContext::GetActiveScene()->GetECSWorld());
	}

	EditorLayer& EditorLayer::GetInstance()
	{
		FLARE_CORE_ASSERT(s_Instance != nullptr, "Invalid EditorLayer instance");
		return *s_Instance;
	}
}