#include "EditorLayer.h"

#include "FlareCore/Profiler/Profiler.h"
#include "FlareCore/Serialization/SerializationStream.h"

#include "Flare.h"
#include "Flare/Core/Application.h"
#include "Flare/Core/Time.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/Font.h"

#include "Flare/Renderer/PostProcessing/ToneMapping.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Project/Project.h"

#include "Flare/Scripting/ScriptingEngine.h"
#include "Flare/Input/InputManager.h"

#include "FlarePlatform/Platform.h"
#include "FlarePlatform/Events.h"

#include "FlareEditor/Serialization/SceneSerializer.h"
#include "FlareEditor/Serialization/YAMLSerialization.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/AssetManager/EditorShaderCache.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/UI/EditorTitleBar.h"
#include "FlareEditor/UI/ProjectSettingsWindow.h"
#include "FlareEditor/UI/ECS/ECSInspector.h"
#include "FlareEditor/UI/PrefabEditor.h"
#include "FlareEditor/UI/SceneViewportWindow.h"
#include "FlareEditor/UI/SerializablePropertyRenderer.h"
#include "FlareEditor/UI/ShaderLibraryWindow.h"

#include "FlareEditor/Scripting/BuildSystem/BuildSystem.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <string_view>

namespace Flare
{
    EditorLayer* EditorLayer::s_Instance = nullptr;

    EditorLayer::EditorLayer()
        : Layer("EditorLayer"), 
        m_EditedSceneHandle(NULL_ASSET_HANDLE), 
        m_PropertiesWindow(m_AssetManagerWindow),
        m_PlaymodePaused(false),
        m_Mode(EditorMode::Edit),
        m_ProjectFilesWacher(nullptr)
    {
        FLARE_PROFILE_FUNCTION();
        s_Instance = this;

        Project::OnProjectOpen.Bind(FLARE_BIND_EVENT_CALLBACK(OnOpenProject));
        Project::OnUnloadActiveProject.Bind([this]()
        {
			EditorAssetManager::GetInstance()->SerializeRegistry();

            m_ProjectFilesWacher.reset();
            if (Scene::GetActive() == nullptr)
                return;

            Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());

            ResetViewportRenderGraphs();

            assetManager->UnloadAsset(Scene::GetActive()->Handle);

			Scene::SetActive(nullptr);
            m_PostProcessingWindow = PostProcessingWindow();

            ScriptingEngine::UnloadAllModules();
            m_ECSContext.Clear();
        });
    }

    EditorLayer::~EditorLayer()
    {
        s_Instance = nullptr;
    }

    void EditorLayer::OnAttach()
    {
        FLARE_PROFILE_FUNCTION();
        ShaderCacheManager::SetInstance(CreateScope<EditorShaderCache>());
        EditorGUI::Initialize();

        m_ImGuiLayer = ImGuiLayer::Create();
        m_PropertiesWindow.OnAttach();

        m_ImGuiLayer->OnAttach();

        Ref<Font> defaultFont = CreateRef<Font>("assets/Fonts/Roboto/Roboto-Regular.ttf");
        Font::SetDefault(defaultFont);

        AssetManager::Intialize(CreateRef<EditorAssetManager>());

        m_AssetManagerWindow.SetOpenAction(AssetType::Scene, [this](AssetHandle handle)
        {
            OpenScene(handle);
        });

        m_GameWindow = CreateRef<ViewportWindow>(m_SceneRenderer, "Game");

        m_ViewportWindows.emplace_back(CreateRef<SceneViewportWindow>(m_Camera, m_SceneRenderer));
        m_ViewportWindows.emplace_back(m_GameWindow);

        Renderer::SetMainViewport(m_GameWindow->GetViewport());

        EditorCameraSettings& settings = m_Camera.GetSettings();
        settings.FOV = 60.0f;
        settings.Near = 0.1f;
        settings.Far = 1000.0f;
        settings.RotationSpeed = 1.0f;

        if (Application::GetInstance().GetCommandLineArguments().ArgumentsCount >= 2)
        {
			const std::string_view projectArgument = "--project=";
            std::optional<std::filesystem::path> projectPath;

            const auto& commandLineArgs = Application::GetInstance().GetCommandLineArguments();
            for (uint32_t i = 0; i < commandLineArgs.ArgumentsCount; i++)
            {
                std::string_view argument = commandLineArgs.Arguments[i];

                if (argument._Starts_with(projectArgument))
                {
                    projectPath = argument.substr(projectArgument.size());
                    break;
                }
            }

            if (projectPath)
            {
				Project::OpenProject(*projectPath);
            }
        }
        else
        {
            std::optional<std::filesystem::path> projectPath = Platform::ShowOpenFileDialog(
                L"Flare Project (*.flareproj)\0*.flareproj\0",
                Application::GetInstance().GetWindow());

            if (projectPath.has_value())
                Project::OpenProject(projectPath.value());
        }

        m_SpriteEditor = CreateRef<SpriteEditor>();

        m_AssetEditorWindows.push_back(m_SpriteEditor);

        m_AssetManagerWindow.SetOpenAction(AssetType::Prefab, [this](AssetHandle handle)
        {
			if (m_PrefabEditor == nullptr)
			{
				m_PrefabEditor = CreateRef<PrefabEditor>(m_ECSContext);
				m_PrefabEditor->OnAttach();

                m_AssetEditorWindows.push_back(m_PrefabEditor);
			}

            m_PrefabEditor->Open(handle);
        });

        m_AssetManagerWindow.SetOpenAction(AssetType::Sprite, [this](AssetHandle handle)
        {
            m_SpriteEditor->Open(handle);
        });

        for (auto& viewportWindow : m_ViewportWindows)
            viewportWindow->OnAttach();
    }

    void EditorLayer::OnDetach()
    {
        FLARE_PROFILE_FUNCTION();
        EditorAssetManager::GetInstance()->SerializeRegistry();

        m_SceneRenderer.reset();

        m_AssetManagerWindow.Uninitialize();
        m_AssetEditorWindows.clear();

        m_ImGuiLayer->OnDetach();

        if (m_Mode == EditorMode::Play)
            ExitPlayMode();

        if (Scene::GetActive() != nullptr && AssetManager::IsAssetHandleValid(Scene::GetActive()->Handle))
            As<EditorAssetManager>(AssetManager::GetInstance())->UnloadAsset(Scene::GetActive()->Handle);

        if (m_PrefabEditor)
        m_PrefabEditor->OnDetach();

        m_PrefabEditor = nullptr;

        m_ViewportWindows.clear();
        m_GameWindow = nullptr;

        m_PostProcessingWindow = PostProcessingWindow();

        EditorGUI::Uninitialize();

        Scene::SetActive(nullptr);
    }

    void EditorLayer::OnUpdate(float deltaTime)
    {
        FLARE_PROFILE_FUNCTION();

        if (m_Mode == EditorMode::Play)
        {
            if (m_GameWindow->HasFocusChanged())
            {
                if (!m_GameWindow->IsFocused())
                    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
                else
                    m_UpdateCursorModeNextFrame = true;
            }

            if (m_UpdateCursorModeNextFrame && !m_PlaymodePaused)
            {
                Application::GetInstance().GetWindow()->SetCursorMode(InputManager::GetCursorMode());
                m_UpdateCursorModeNextFrame = false;

                switch (InputManager::GetCursorMode())
                {
                case CursorMode::Hidden:
                case CursorMode::Disabled:
                    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
                    break;
                }
            }
        }

        Renderer2D::ResetStats();
        Renderer::ClearStatistics();

        Renderer::SetMainViewport(m_GameWindow->GetViewport());
        InputManager::SetMousePositionOffset(-m_GameWindow->GetViewport().GetPosition());

        {
            FLARE_PROFILE_SCOPE("Scene Runtime Update");

            Ref<Scene> activeScene = Scene::GetActive();

            if (m_Mode == EditorMode::Play && !m_PlaymodePaused)
                activeScene->OnUpdateRuntime();
            else if (m_Mode == EditorMode::Edit && activeScene)
                activeScene->OnUpdateEditor();
        }

        if (m_SceneRenderer)
        {
			m_SceneRenderer->CollectSceneData();
        }

        {
            FLARE_PROFILE_SCOPE("Viewport Render");
            for (auto& viewport : m_ViewportWindows)
            {
                viewport->OnRenderViewport();
            }
        }

        if (m_ProjectFilesWacher)
        {
            FLARE_PROFILE_SCOPE("WatchForFileChanges");
            m_ProjectFilesWacher->Update();
            FileChangeEvent changes;

            Ref<EditorAssetManager> editorAssetManager = As<EditorAssetManager>(AssetManager::GetInstance());

            bool shouldRebuildAssetTree = false;
            while (true)
            {
                auto result = m_ProjectFilesWacher->TryGetNextEvent(changes);
                if (result != FileWatcher::Result::Ok)
                    break;

                switch (changes.Action)
                {
                case FileChangeEvent::ActionType::Created:
                    shouldRebuildAssetTree = true;
                    break;
                case FileChangeEvent::ActionType::Deleted:
                    shouldRebuildAssetTree = true;
                    break;
                case FileChangeEvent::ActionType::Renamed:
                    shouldRebuildAssetTree = true;
                    break;
                case FileChangeEvent::ActionType::Modified:
                    std::filesystem::path absoluteFilePath = Project::GetActive()->Location / changes.FilePath;
                    std::optional<AssetHandle> handle = editorAssetManager->FindAssetByPath(absoluteFilePath);

                    bool isValid = handle.has_value() && AssetManager::IsAssetHandleValid(handle.value());
                    if (isValid && AssetManager::IsAssetLoaded(handle.value_or(NULL_ASSET_HANDLE)))
                    {
                        const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle.value());
                        FLARE_CORE_ASSERT(metadata);

                        if (metadata->Type == AssetType::Shader)
                        {
                            if (Application::GetInstance().GetWindow()->GetProperties().IsFocused)
                                editorAssetManager->ReloadAsset(*handle);
                            else
                                m_AssetReloadQueue.emplace(*handle);
                        }
                    }

                    break;
                }
            }

            if (shouldRebuildAssetTree)
            {
                m_AssetManagerWindow.RebuildAssetTree();
            }
        }

        {
            EditorAssetManager& assetManager = *EditorAssetManager::GetInstance();
            if (assetManager.GetRegistry().IsDirty())
            {
                assetManager.SerializeRegistry();
            }
        }
    }

    void EditorLayer::OnEvent(Event& event)
    {
        FLARE_PROFILE_FUNCTION();
        for (Ref<ViewportWindow>& window : m_ViewportWindows)
            window->OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& e) -> bool
        {
            if (e.GetKeyCode() == KeyCode::Escape)
                Application::GetInstance().GetWindow()->SetCursorMode(CursorMode::Normal);
            return false;
        });

        dispatcher.Dispatch<WindowFocusEvent>([this](WindowFocusEvent& e) -> bool
        {
            if (e.IsFocused())
            {
                Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
                for (AssetHandle handle : m_AssetReloadQueue)
                    assetManager->ReloadAsset(handle);

                m_AssetReloadQueue.clear();
                m_AssetManagerWindow.RebuildAssetTree();
            }
            return false;
        });

        if (!event.Handled && m_PrefabEditor)
        {
            m_PrefabEditor->OnEvent(event);
        }
        
        // InputManager only works with Game viewport,
        // so the events should only be processed when the Game window is focused
        if (!event.Handled && m_GameWindow->IsFocused())
            InputManager::ProcessEvent(event);
    }

    void EditorLayer::OnImGUIRender()
    {
        FLARE_PROFILE_FUNCTION();

        if (ImGui::IsKeyPressed(ImGuiKey_F10))
        {
            SetFullscreenViewportWindow(nullptr);
        }

        HandleKeyboardShortcuts();

        if (m_FullscreenViewport)
        {
            m_ImGuiLayer->Begin();

            m_FullscreenViewport->OnRenderImGui();

            m_ImGuiLayer->End();
			m_ImGuiLayer->RenderCurrentWindow();
			m_ImGuiLayer->UpdateWindows();

            return;
        }

        m_ImGuiLayer->Begin();
        m_ImGuiLayer->BeginDockSpace();

        m_TitleBar.OnRenderImGui();

        {
            ImGui::Begin("Renderer");
            const auto& stats = Renderer::GetStatistics();

            ImGui::Text("Frame time: %f ms", Time::GetDeltaTime() * 1000.0f);
            ImGui::Text("FPS: %f", 1.0f / Time::GetDeltaTime());

            Ref<Window> window = Application::GetInstance().GetWindow();

            bool vsync = window->GetProperties().VSyncEnabled;
            if (ImGui::Checkbox("VSync", &vsync))
                window->SetVSync(vsync);

            ImGui::SeparatorText("Renderer");
            {
                ImGui::Text("Geometry Pass: %f ms", stats.GeometryPassTime);
                ImGui::Text("Shadow Pass: %f ms", stats.ShadowPassTime);
                ImGui::Text("Objects Submitted: %d Objects Visible: %d", stats.ObjectsSubmitted, stats.ObjectsVisible);
                ImGui::Text("Draw calls (Saved by instancing: %d): %d", stats.DrawCallsSavedByInstancing, stats.DrawCallCount);
            }

            ImGui::SeparatorText("Renderer 2D");
            {
                const auto& stats = Renderer2D::GetStats();
                ImGui::Text("Quads: %d", stats.QuadsCount);
                ImGui::Text("Draw Calls: %d", stats.DrawCalls);
                ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
            }

            ImGui::End();
        }

        {
            FLARE_PROFILE_SCOPE("ViewportWindows ImGui");
            for (auto& viewport : m_ViewportWindows)
                viewport->OnRenderImGui();
        }

        {
            FLARE_PROFILE_SCOPE("EditorWindowsUpdate");
            
            ProjectSettingsWindow::OnRenderImGui();
            ShaderLibraryWindow::GetInstance().OnRenderImGui();
            m_SceneWindow.OnImGuiRender();
            m_PropertiesWindow.OnImGuiRender();
            m_AssetManagerWindow.OnImGuiRender();
            m_QuickSearch.OnImGuiRender();
            m_PostProcessingWindow.OnImGuiRender();

            ECSInspector::GetInstance().OnImGuiRender();

            for (auto& window : m_AssetEditorWindows)
            {
                if (!window)
                    continue;

                window->OnUpdate();
            }
        }

        m_ImGuiLayer->EndDockSpace();

        m_ImGuiLayer->End();
        m_ImGuiLayer->RenderCurrentWindow();
        m_ImGuiLayer->UpdateWindows();
    }

    void EditorLayer::UpdateWindowTitle()
    {
		FLARE_PROFILE_FUNCTION();
        if (Project::GetActive() != nullptr)
        {
            std::string name = fmt::format("Flare Editor - {0} - {1}", Project::GetActive()->Name, Project::GetActive()->Location.generic_string());
            Application::GetInstance().GetWindow()->SetTitle(name);
        }
    }

    void EditorLayer::OnOpenProject()
    {
		FLARE_PROFILE_FUNCTION();
        Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());

        assetManager->Reinitialize();

        UpdateWindowTitle();
        m_AssetManagerWindow.RebuildAssetTree();

        AssetHandle startScene = Project::GetActive()->StartScene;

        ScriptingEngine::LoadModules();
        m_ECSContext.Components.RegisterComponents();

        OpenSceneImmediately(startScene);
        assetManager->ReloadPrefabs(); // HACK: component ids have changed after reregistering components, so reload prefabs

        m_ProjectFilesWacher.reset(FileWatcher::Create(Project::GetActive()->Location, EventsMask::FileName | EventsMask::DirectoryName | EventsMask::LastWrite));
    }

    void EditorLayer::OpenSceneImmediately(AssetHandle handle)
    {
		FLARE_PROFILE_FUNCTION();
        if (!AssetManager::IsAssetHandleValid(handle))
            return;

		ResetViewportRenderGraphs();

        m_SceneRenderer.reset();

		Ref<Scene> active = Scene::GetActive();

		Ref<EditorAssetManager> editorAssetManager = As<EditorAssetManager>(AssetManager::GetInstance());

		if (active != nullptr && AssetManager::IsAssetHandleValid(active->Handle))
			editorAssetManager->UnloadAsset(active->Handle);

		active = nullptr;
		Scene::SetActive(nullptr);

		active = AssetManager::GetAsset<Scene>(handle);
		Scene::SetActive(active);

		active->InitializeRuntime();

		m_EditedSceneHandle = handle;

		m_PostProcessingWindow = PostProcessingWindow(active);

        m_SceneRenderer.reset(new SceneRenderer(active));
    }

    void EditorLayer::HandleKeyboardShortcuts()
    {
        FLARE_PROFILE_FUNCTION();
        ImGuiIO& io = ImGui::GetIO();

        if (io.KeyCtrl)
        {
			if (ImGui::IsKeyPressed(ImGuiKey_P))
			{
				if (m_Mode == EditorMode::Edit)
				{
					EnterPlayMode();
				}
				else
				{
					ExitPlayMode();
				}
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_S))
			{
				if (io.KeyShift)
				{
					SaveActiveSceneAs();
				}
				else
				{
					SaveActiveScene();
				}
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_N))
			{
                CreateNewScene();
			}
        }
    }

    void EditorLayer::ResetViewportRenderGraphs()
    {
		FLARE_PROFILE_FUNCTION();
        for (auto& viewportWindow : m_ViewportWindows)
        {
            viewportWindow->GetViewport().Graph.Clear();
            viewportWindow->GetViewport().Graph.SetNeedsRebuilding();
        }
    }

    void EditorLayer::SaveActiveScene()
    {
		FLARE_PROFILE_FUNCTION();
        FLARE_CORE_ASSERT(Scene::GetActive());
        if (AssetManager::IsAssetHandleValid(Scene::GetActive()->Handle))
            SceneSerializer::Serialize(Scene::GetActive(), m_Camera, m_SceneViewSettings);
        else
            SaveActiveSceneAs();
    }

    void EditorLayer::SaveActiveSceneAs()
    {
		FLARE_PROFILE_FUNCTION();
        Application::GetInstance().ExecuteAfterEndOfFrame([this]()
		{
			GraphicsContext::GetInstance().WaitForDevice();

			std::optional<std::filesystem::path> scenePath = Platform::ShowSaveFileDialog(
				L"Flare Scene (*.flare)\0*.flare\0",
				Application::GetInstance().GetWindow());

			if (scenePath.has_value())
			{
				std::filesystem::path& path = scenePath.value();
				if (!path.has_extension())
					path.replace_extension(".flare");

				SceneSerializer::Serialize(Scene::GetActive(), path, m_Camera, m_SceneViewSettings);
				AssetHandle handle = As<EditorAssetManager>(AssetManager::GetInstance())->ImportAsset(path);
				OpenSceneImmediately(handle);
			}
		});
    }

    void EditorLayer::OpenScene(AssetHandle handle)
    {
		FLARE_PROFILE_FUNCTION();
        if (!AssetManager::IsAssetHandleValid(handle))
            return;

        Application::GetInstance().ExecuteAfterEndOfFrame([this, handle]()
		{
			GraphicsContext::GetInstance().WaitForDevice();
			OpenSceneImmediately(handle);
		});
    }

    void EditorLayer::CreateNewScene()
    {
		FLARE_PROFILE_FUNCTION();
        Application::GetInstance().ExecuteAfterEndOfFrame([this]()
		{
			GraphicsContext::GetInstance().WaitForDevice();

			Ref<Scene> active = Scene::GetActive();

			ResetViewportRenderGraphs();

			if (active != nullptr)
			{
				Ref<EditorAssetManager> editorAssetManager = As<EditorAssetManager>(AssetManager::GetInstance());

				if (active != nullptr && AssetManager::IsAssetHandleValid(active->Handle))
					editorAssetManager->UnloadAsset(active->Handle);
			}

			active = nullptr;

			active = CreateRef<Scene>(m_ECSContext);
			active->Initialize();
			active->InitializeRuntime();
			Scene::SetActive(active);

			m_EditedSceneHandle = NULL_ASSET_HANDLE;
		});
    }

    void EditorLayer::EnterPlayMode()
    {
		FLARE_PROFILE_FUNCTION();
        FLARE_CORE_ASSERT(m_Mode == EditorMode::Edit);

        if (m_EnterPlayModeScheduled)
            return;

        m_EnterPlayModeScheduled = true;

		Application::GetInstance().ExecuteAfterEndOfFrame([this]()
		{
			GraphicsContext::GetInstance().WaitForDevice();
			ResetViewportRenderGraphs();

			m_GameWindow->RequestFocus();
			m_UpdateCursorModeNextFrame = true;

			Ref<Scene> active = Scene::GetActive();

			Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
			std::filesystem::path activeScenePath = assetManager->GetAssetMetadata(active->Handle)->Path;

			SaveActiveScene();

			m_PlaymodePaused = false;

			Scene::SetActive(nullptr);
			assetManager->UnloadAsset(active->Handle);
			active = nullptr;

			Ref<Scene> playModeScene = CreateRef<Scene>(m_ECSContext);
			SceneSerializer::Deserialize(playModeScene, activeScenePath, m_Camera, m_SceneViewSettings);

			Scene::SetActive(playModeScene);
			m_Mode = EditorMode::Play;

			playModeScene->InitializeRuntime();
			Scene::GetActive()->OnRuntimeStart();

            m_EnterPlayModeScheduled = false;
		});
    }

    void EditorLayer::ExitPlayMode()
    {
		FLARE_PROFILE_FUNCTION();
        FLARE_CORE_ASSERT(m_Mode == EditorMode::Play);

        if (m_ExitPlayModeScheduled)
            return;

        m_ExitPlayModeScheduled = true;

        Application::GetInstance().ExecuteAfterEndOfFrame([this]()
		{
			GraphicsContext::GetInstance().WaitForDevice();
			Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());

			Scene::GetActive()->OnRuntimeEnd();

			ResetViewportRenderGraphs();

			Scene::SetActive(nullptr);

			Ref<Scene> editorScene = AssetManager::GetAsset<Scene>(m_EditedSceneHandle);
			editorScene->InitializeRuntime();

			Scene::SetActive(editorScene);
			m_Mode = EditorMode::Edit;

			InputManager::SetCursorMode(CursorMode::Normal);
            m_ExitPlayModeScheduled = false;
		});
    }

    void EditorLayer::ReloadScriptingModules()
    {
		FLARE_PROFILE_FUNCTION();
        FLARE_CORE_ASSERT(!Platform::IsDebuggerAttached());
        FLARE_CORE_ASSERT(m_Mode == EditorMode::Edit);

        Ref<Scene> active = Scene::GetActive();
        AssetHandle activeSceneHandle = active->Handle;

        Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
        std::filesystem::path activeScenePath = assetManager->GetAssetMetadata(active->Handle)->Path;
        SaveActiveScene();

        ResetViewportRenderGraphs();

        Scene::SetActive(nullptr);
        assetManager->UnloadAsset(active->Handle);
        active = nullptr;

        ScriptingEngine::UnloadAllModules();

        FLARE_CORE_INFO("Compiling...");
        BuildSystem::BuildModules();
        FLARE_CORE_INFO("Linking...");
        BuildSystem::LinkModules();

        ScriptingEngine::LoadModules();
        m_ECSContext.Components.ReregisterComponents();

        active = CreateRef<Scene>(m_ECSContext);
        active->Handle = activeSceneHandle;
        SceneSerializer::Deserialize(active, activeScenePath, m_Camera, m_SceneViewSettings);

        active->GetECSWorld().GetSystemsManager().RegisterSystems();
        active->InitializeRuntime();
        Scene::SetActive(active);

        assetManager->ReloadPrefabs();
    }

    void EditorLayer::SetFullscreenViewportWindow(Ref<ViewportWindow> viewportWindow)
    {
        if (m_FullscreenViewport)
        {
            m_FullscreenViewport->SetMaximized(false);
        }

        m_FullscreenViewport = viewportWindow;

        if (m_FullscreenViewport)
        {
			m_FullscreenViewport->SetMaximized(true);
        }
    }

    EditorLayer& EditorLayer::GetInstance()
    {
        FLARE_CORE_ASSERT(s_Instance != nullptr, "Invalid EditorLayer instance");
        return *s_Instance;
    }
}
