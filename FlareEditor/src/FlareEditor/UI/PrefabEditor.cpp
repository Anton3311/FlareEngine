#include "PCH.h"

#include "PrefabEditor.h"

#include "Flare/Scene/Prefab.h"
#include "Flare/Scene/Scene.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Scene/SceneRenderer.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"

#include "FlareEditor/AssetManager/PrefabImporter.h"

#include <imgui.h>

namespace Flare
{
    static constexpr EntitiesHierarchyFeatures DEFAULT_HIERARCHY_FEATURES = EntitiesHierarchyFeatures::CreateEntity
        | EntitiesHierarchyFeatures::DeleteEntity
        | EntitiesHierarchyFeatures::DuplicateEntity;

    static constexpr EntitiesHierarchyFeatures GENERATED_HIERARHCY_FEATURES = EntitiesHierarchyFeatures::None;

    PrefabEditor::PrefabEditor(ECSContext& context)
        : m_PreviewScene(Ref<Scene>::New(context)),
        m_Entities(GetWorld(), DEFAULT_HIERARCHY_FEATURES),
        m_Properties(GetWorld()), m_SelectedEntity(Entity()),
        m_ViewportWindow(m_SceneRenderer, m_SceneViewSettings, "Prefab Preview")
    {
        FLARE_PROFILE_FUNCTION();
        m_SceneRenderer.reset(new SceneRenderer(m_PreviewScene));
        m_SceneRenderer->SetDefaultDirectionalLight(glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f)), glm::vec3(1.0f), 2.0f);
        m_SceneRenderer->SetDefaultEnvironmentLight(glm::vec3(1.0f), 0.8f);

        m_SceneViewSettings.ShowAABBs = false;
        m_SceneViewSettings.ShowCameraFrustum = false;
        m_SceneViewSettings.ShowLights = true;
        m_SceneViewSettings.ShowGrid = true;

        m_ViewportWindow.SetScene(m_PreviewScene);
        Entity viewportEntity = m_ViewportWindow.GetViewportEntity();
        World& renderWorld = Renderer::GetRenderWorld();

        Viewport& viewport = renderWorld.GetEntityComponent<Viewport>(viewportEntity);
        viewport.Settings.ShadowMappingEnabled = false;
        viewport.Settings.PostProcessingEnabled = false;

        m_PreviewScene->InitializeRuntime();
    }

    void PrefabEditor::OnOpen(AssetHandle asset)
    {
        FLARE_PROFILE_FUNCTION();
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(asset));
        m_Prefab = AssetManager::GetAsset<Prefab>(asset);

        if (m_Prefab)
        {
			m_Prefab->TryCreateInstance(GetWorld());

            bool isGenerated = HAS_BIT(m_Prefab->GetFlags(), PrefabFlags::Generated);
            m_Entities.SetFeatures(isGenerated ? GENERATED_HIERARHCY_FEATURES : DEFAULT_HIERARCHY_FEATURES);

			World& renderWorld = Renderer::GetRenderWorld();
			renderWorld.GetEntityComponent<ViewportRenderGraph>(m_ViewportWindow.GetViewportEntity()).Graph->SetNeedsRebuilding();

			m_ViewportWindow.ShowWindow = true;
		}
        else
        {
            FLARE_CORE_ERROR("Cannot open prefab, because it is null");
        }
    }

    void PrefabEditor::OnClose()
    {
        FLARE_PROFILE_FUNCTION();

        if (!m_Prefab)
        {
			FLARE_CORE_ERROR("Cannot save the prefab, because it is null");
            return;
        }

		World& world = GetWorld();
        if (!HAS_BIT(m_Prefab->GetFlags(), PrefabFlags::Generated))
        {
			AssetHandle prefabHandle = m_Prefab->Handle;
			m_Prefab->GetHierarchy().CopyFromWorld(world);

			PrefabImporter::SerializePrefab(prefabHandle);
        }

        m_Prefab = nullptr;
        world.Entities.Clear();
    }

    void PrefabEditor::OnRenderImGui(bool& show)
    {
        FLARE_PROFILE_FUNCTION();
        m_PreviewScene->OnUpdate();

        m_SceneRenderer->CollectSceneData();

        m_ViewportWindow.OnRenderViewport(Renderer::GetRenderWorld());
        m_ViewportWindow.OnRenderImGui();

		ImGui::Begin("Prefab Entities");
		m_Entities.OnRenderImGui(m_SelectedEntity);
		ImGui::End();

		ImGui::Begin("Prefab Entity Properties");
		bool isGenerated = HAS_BIT(m_Prefab->GetFlags(), PrefabFlags::Generated);
		m_Properties.OnRenderImGui(m_SelectedEntity, isGenerated);
		ImGui::End();

        show = m_ViewportWindow.ShowWindow;
    }

    World& PrefabEditor::GetWorld()
    {
		return m_PreviewScene->GetECSWorld();
    }

    void PrefabEditor::OnAttach()
    {
        FLARE_PROFILE_FUNCTION();
        m_ViewportWindow.OnAttach();
    }

    void PrefabEditor::OnEvent(Event& event)
    {
        FLARE_PROFILE_FUNCTION();
        m_ViewportWindow.OnEvent(event);
    }
}