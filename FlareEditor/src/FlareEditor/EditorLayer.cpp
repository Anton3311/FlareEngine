#include "EditorLayer.h"

#include "FlareCore/Profiler/Profiler.h"
#include "FlareCore/Serialization/SerializationStream.h"
#include "FlareCore/Serialization/Serializer.h"

#include "Flare.h"
#include "Flare/Core/Application.h"
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
#include "FlareEditor/UI/SceneViewportWindow.h"
#include "FlareEditor/UI/EditorTitleBar.h"
#include "FlareEditor/UI/ProjectSettingsWindow.h"
#include "FlareEditor/UI/ECS/ECSInspector.h"
#include "FlareEditor/UI/PrefabEditor.h"
#include "FlareEditor/UI/SerializablePropertyRenderer.h"

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
        m_ProjectFilesWacher(nullptr),
        Guizmo(GuizmoMode::None)
    {
        s_Instance = this;

        Project::OnProjectOpen.Bind(FLARE_BIND_EVENT_CALLBACK(OnOpenProject));
        Project::OnUnloadActiveProject.Bind([this]()
        {
            m_ProjectFilesWacher.reset();
            if (Scene::GetActive() == nullptr)
                return;

            Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());

            assetManager->UnloadAsset(Scene::GetActive()->Handle);
            Scene::SetActive(nullptr);
        });
    }

    EditorLayer::~EditorLayer()
    {
        s_Instance = nullptr;
    }

    void EditorLayer::OnAttach()
    {
        ShaderCacheManager::SetInstance(CreateScope<EditorShaderCache>());

        m_PropertiesWindow.OnAttach();
        ImGuiLayer::OnAttach();

        Ref<Font> defaultFont = CreateRef<Font>("assets/Fonts/Roboto/Roboto-Regular.ttf");
        Font::SetDefault(defaultFont);

        AssetManager::Intialize(CreateRef<EditorAssetManager>());

        m_AssetManagerWindow.SetOpenAction(AssetType::Scene, [this](AssetHandle handle)
        {
            OpenScene(handle);
        });

        m_GameWindow = CreateRef<ViewportWindow>("Game");
        m_ViewportWindows.emplace_back(CreateRef<SceneViewportWindow>(m_Camera));
        m_ViewportWindows.emplace_back(m_GameWindow);

        EditorCameraSettings& settings = m_Camera.GetSettings();
        settings.FOV = 60.0f;
        settings.Near = 0.1f;
        settings.Far = 1000.0f;
        settings.RotationSpeed = 1.0f;

        if (Application::GetInstance().GetCommandLineArguments().ArgumentsCount >= 2)
        {
            std::filesystem::path projectPath = Application::GetInstance().GetCommandLineArguments()[1];
            Project::OpenProject(projectPath);
        }
        else
        {
            std::optional<std::filesystem::path> projectPath = Platform::ShowOpenFileDialog(
                L"Flare Project (*.flareproj)\0*.flareproj\0",
                Application::GetInstance().GetWindow());

            if (projectPath.has_value())
                Project::OpenProject(projectPath.value());
        }

        m_PrefabEditor = CreateRef<PrefabEditor>(m_ECSContext);
        m_AssetEditorWindows.push_back(m_PrefabEditor);

        m_AssetManagerWindow.SetOpenAction(AssetType::Prefab, [this](AssetHandle handle)
        {
            m_PrefabEditor->Open(handle);
        });

        for (auto& viewportWindow : m_ViewportWindows)
            viewportWindow->OnAttach();
    }

    void EditorLayer::OnDetach()
    {
        ImGuiLayer::OnDetach();

        if (m_Mode == EditorMode::Play)
            ExitPlayMode();

        if (Scene::GetActive() != nullptr && AssetManager::IsAssetHandleValid(Scene::GetActive()->Handle))
            As<EditorAssetManager>(AssetManager::GetInstance())->UnloadAsset(Scene::GetActive()->Handle);

        Scene::SetActive(nullptr);
    }

    void EditorLayer::OnUpdate(float deltaTime)
    {
        FLARE_PROFILE_FUNCTION();
        m_PreviousFrameTime = deltaTime;

        Renderer2D::ResetStats();
        Renderer::ClearStatistics();

        Renderer::SetMainViewport(m_GameWindow->GetViewport());
        InputManager::SetMousePositionOffset(-m_GameWindow->GetViewport().GetPosition());

        {
            FLARE_PROFILE_SCOPE("Scene Runtime Update");

            if (m_Mode == EditorMode::Play && !m_PlaymodePaused)
                Scene::GetActive()->OnUpdateRuntime();
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
                            editorAssetManager->ReloadAsset(handle.value());
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
    }

    void EditorLayer::OnEvent(Event& event)
    {
        FLARE_PROFILE_FUNCTION();
        for (Ref<ViewportWindow>& window : m_ViewportWindows)
            window->OnEvent(event);
        
        // InputManager only works with Game viewport,
        // so the events should only be processed when the Game window is focused
        if (!event.Handled && m_GameWindow->IsFocused())
			InputManager::ProcessEvent(event);
    }

    void EditorLayer::OnImGUIRender()
    {
        FLARE_PROFILE_FUNCTION();
        ImGuiLayer::Begin();

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

        m_TitleBar.OnRenderImGui();

        {
            ImGui::Begin("Shadows");

            static int32_t s_CascadeIndex = 0;

            ShadowSettings& settings = Renderer::GetShadowSettings();

            ImGui::SliderInt("Cascade Index", &s_CascadeIndex, 0, settings.Cascades - 1, "%d", ImGuiSliderFlags_AlwaysClamp);

            auto shadows = Renderer::GetShadowsRenderTarget(s_CascadeIndex);
            if (shadows)
                ImGui::Image(shadows->GetColorAttachmentRendererId(0), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));

            ImGui::End();
        }

        {
            ImGui::Begin("Renderer");

            ImGui::Text("Frame time %f", m_PreviousFrameTime);
            ImGui::Text("FPS %f", 1.0f / m_PreviousFrameTime);

            Ref<Window> window = Application::GetInstance().GetWindow();

            bool vsync = window->GetProperties().VSyncEnabled;
            if (ImGui::Checkbox("VSync", &vsync))
                window->SetVSync(vsync);

            if (ImGui::ColorEdit3("Clear Color", glm::value_ptr(m_ClearColor), ImGuiColorEditFlags_Float))
                RenderCommand::SetClearColor(m_ClearColor.r, m_ClearColor.g, m_ClearColor.b, 1.0);

            if (ImGui::CollapsingHeader("Renderer 2D"))
            {
                const auto& stats = Renderer2D::GetStats();
                ImGui::Text("Quads %d", stats.QuadsCount);
                ImGui::Text("Draw Calls %d", stats.DrawCalls);
                ImGui::Text("Vertices %d", stats.GetTotalVertexCount());
            }

            if (ImGui::CollapsingHeader("Renderer"))
            {
                const auto& stats = Renderer::GetStatistics();
                ImGui::Text("Submitted: %d Culled: %d", stats.ObjectsSubmitted, stats.ObjectsCulled);
                ImGui::Text("Draw calls (Saved by instancing: %d): %d", stats.DrawCallsSavedByInstancing, stats.DrawCallsCount);
            }

            if (ImGui::TreeNodeEx("Post Processing", ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth))
            {
                auto& postProcessing = Scene::GetActive()->GetPostProcessingManager();

                EditorGUI::ObjectField(
                    FLARE_SERIALIZATION_DESCRIPTOR_OF(ToneMapping),
                    postProcessing.ToneMappingPass.get());
                EditorGUI::ObjectField(
                    FLARE_SERIALIZATION_DESCRIPTOR_OF(Vignette),
                    postProcessing.VignettePass.get());
                EditorGUI::ObjectField(
                    FLARE_SERIALIZATION_DESCRIPTOR_OF(SSAO),
                    postProcessing.SSAOPass.get());

                ImGui::TreePop();
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
            m_SceneWindow.OnImGuiRender();
            m_PropertiesWindow.OnImGuiRender();
            m_AssetManagerWindow.OnImGuiRender();
            m_QuickSearch.OnImGuiRender();
            m_ProfilerWindow.OnImGuiRender();

            ECSInspector::GetInstance().OnImGuiRender();

            for (auto& window : m_AssetEditorWindows)
                window->OnUpdate();
        }

        ImGui::End();
        ImGuiLayer::End();
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

        UpdateWindowTitle();
        m_AssetManagerWindow.RebuildAssetTree();

        AssetHandle startScene = Project::GetActive()->StartScene;
        OpenScene(startScene);

        m_ProjectFilesWacher.reset(FileWatcher::Create(Project::GetActive()->Location, EventsMask::FileName | EventsMask::DirectoryName | EventsMask::LastWrite));
    }

    void EditorLayer::SaveActiveScene()
    {
        FLARE_CORE_ASSERT(Scene::GetActive());
        if (AssetManager::IsAssetHandleValid(Scene::GetActive()->Handle))
            SceneSerializer::Serialize(Scene::GetActive());
        else
            SaveActiveSceneAs();
    }

    void EditorLayer::SaveActiveSceneAs()
    {
        std::optional<std::filesystem::path> scenePath = Platform::ShowSaveFileDialog(
            L"Flare Scene (*.flare)\0*.flare\0",
            Application::GetInstance().GetWindow());

        if (scenePath.has_value())
        {
            std::filesystem::path& path = scenePath.value();
            if (!path.has_extension())
                path.replace_extension(".flare");

            SceneSerializer::Serialize(Scene::GetActive(), path);
            AssetHandle handle = As<EditorAssetManager>(AssetManager::GetInstance())->ImportAsset(path);
            OpenScene(handle);
        }
    }

    void EditorLayer::OpenScene(AssetHandle handle)
    {
        if (AssetManager::IsAssetHandleValid(handle))
        {
            Ref<Scene> active = Scene::GetActive();

            Ref<EditorAssetManager> editorAssetManager = As<EditorAssetManager>(AssetManager::GetInstance());

            if (active != nullptr && AssetManager::IsAssetHandleValid(active->Handle))
                editorAssetManager->UnloadAsset(active->Handle);

            active = nullptr;
            Scene::SetActive(nullptr);
            ScriptingEngine::UnloadAllModules();
            m_ECSContext.Clear();

            ScriptingEngine::LoadModules();

            active = AssetManager::GetAsset<Scene>(handle);
            Scene::SetActive(active);

            active->InitializeRuntime();

            m_EditedSceneHandle = handle;
        }
    }

    void EditorLayer::CreateNewScene()
    {
        Ref<Scene> active = Scene::GetActive();

        if (active != nullptr)
        {
            Ref<EditorAssetManager> editorAssetManager = As<EditorAssetManager>(AssetManager::GetInstance());

            if (active != nullptr && AssetManager::IsAssetHandleValid(active->Handle))
                editorAssetManager->UnloadAsset(active->Handle);
        }

        active = nullptr;
        Scene::SetActive(nullptr);
        ScriptingEngine::UnloadAllModules();
        m_ECSContext.Clear();

        ScriptingEngine::LoadModules();

        active = CreateRef<Scene>(m_ECSContext);
        active->Initialize();
        active->InitializeRuntime();
        Scene::SetActive(active);

        m_EditedSceneHandle = 0;
    }

    void EditorLayer::EnterPlayMode()
    {
        FLARE_CORE_ASSERT(m_Mode == EditorMode::Edit);

        Ref<Scene> active = Scene::GetActive();

        Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
        std::filesystem::path activeScenePath = assetManager->GetAssetMetadata(active->Handle)->Path;
        SaveActiveScene();

        m_PlaymodePaused = false;

        Scene::SetActive(nullptr);
        assetManager->UnloadAsset(active->Handle);
        active = nullptr;

        ScriptingEngine::UnloadAllModules();
        m_ECSContext.Clear();

        ScriptingEngine::LoadModules();
        Ref<Scene> playModeScene = CreateRef<Scene>(m_ECSContext);
        SceneSerializer::Deserialize(playModeScene, activeScenePath);

        Scene::SetActive(playModeScene);
        m_Mode = EditorMode::Play;

        assetManager->ReloadPrefabs();

        playModeScene->InitializeRuntime();
        Scene::GetActive()->OnRuntimeStart();
    }

    void EditorLayer::ExitPlayMode()
    {
        FLARE_CORE_ASSERT(m_Mode == EditorMode::Play);
        Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());

        Scene::GetActive()->OnRuntimeEnd();

        Scene::SetActive(nullptr);
        ScriptingEngine::UnloadAllModules();
        m_ECSContext.Clear();


        ScriptingEngine::LoadModules();
        Ref<Scene> editorScene = AssetManager::GetAsset<Scene>(m_EditedSceneHandle);
        editorScene->InitializeRuntime();

        Scene::SetActive(editorScene);
        m_Mode = EditorMode::Edit;

        assetManager->ReloadPrefabs();
    }

    void EditorLayer::ReloadScriptingModules()
    {
        FLARE_CORE_ASSERT(m_Mode == EditorMode::Edit);

        Ref<Scene> active = Scene::GetActive();
        AssetHandle activeSceneHandle = active->Handle;

        Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
        std::filesystem::path activeScenePath = assetManager->GetAssetMetadata(active->Handle)->Path;
        SaveActiveScene();

        Scene::SetActive(nullptr);
        assetManager->UnloadAsset(active->Handle);
        active = nullptr;

        ScriptingEngine::UnloadAllModules();
        m_ECSContext.Clear();

        BuildSystem::BuildModules();

        ScriptingEngine::LoadModules();
        active = CreateRef<Scene>(m_ECSContext);
        active->Handle = activeSceneHandle;
        SceneSerializer::Deserialize(active, activeScenePath);

        active->InitializeRuntime();
        Scene::SetActive(active);
    }

    EditorLayer& EditorLayer::GetInstance()
    {
        FLARE_CORE_ASSERT(s_Instance != nullptr, "Invalid EditorLayer instance");
        return *s_Instance;
    }
}