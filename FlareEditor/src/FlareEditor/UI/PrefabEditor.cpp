#include "PrefabEditor.h"

#include "Flare/AssetManager/AssetManager.h"
#include "FlareEditor/AssetManager/PrefabImporter.h"

#include <imgui.h>

namespace Flare
{
    PrefabEditor::PrefabEditor(ECSContext& context)
        : m_PreviewScene(CreateRef<Scene>(context)),
        m_Entities(GetWorld(), EntitiesHierarchyFeatures::None),
        m_Properties(GetWorld()), m_SelectedEntity(Entity()),
        m_ViewportWindow(m_EditorCamera, "Prefab Preview")
    {
        m_ViewportWindow.SetScene(m_PreviewScene);

        EditorCameraSettings& settings = m_EditorCamera.GetSettings();
        settings.FOV = 60.0f;
        settings.Near = 0.1f;
        settings.Far = 1000.0f;
        settings.RotationSpeed = 1.0f;

        m_PreviewScene->Initialize();
        m_ViewportWindow.GetViewport().PostProcessingEnabled = false;
    }

    void PrefabEditor::OnOpen(AssetHandle asset)
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(asset));
        m_Prefab = AssetManager::GetAsset<Prefab>(asset);
        m_Prefab->CreateInstance(GetWorld());

        m_PreviewScene->InitializeRuntime();

        m_ViewportWindow.ShowWindow = true;
    }

    void PrefabEditor::OnClose()
    {
        AssetHandle prefabHandle = m_Prefab->Handle;
        Entity entity = Entity();
        World& world = GetWorld();
        if (world.Entities.GetEntityRecords().size() > 0)
            entity = world.Entities.GetEntityRecords()[0].Id;

        PrefabImporter::SerializePrefab(prefabHandle, world, entity);

        m_Prefab = nullptr;
        world.Entities.Clear();
    }

    void PrefabEditor::OnRenderImGui(bool& show)
    {
        m_ViewportWindow.OnRenderViewport();
        m_ViewportWindow.OnRenderImGui();

        ImGui::Begin("Prefab Entities");
        m_Entities.OnRenderImGui(m_SelectedEntity);
        ImGui::End();

        ImGui::Begin("Prefab Entity Properties");
        m_Properties.OnRenderImGui(m_SelectedEntity);
        ImGui::End();

        show = m_ViewportWindow.ShowWindow;
    }

    void PrefabEditor::OnEvent(Event& event)
    {
        m_ViewportWindow.OnEvent(event);
    }
}