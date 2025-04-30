#include "PCH.h"

#include "SceneWindow.h"

#include "Flare/Scene/Scene.h"

#include "FlareECS/World.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/EditorLayer.h"

namespace Flare
{
	SceneWindow::SceneWindow()
		: m_Hierarchy(EntitiesHierarchyFeatures::All)
	{
	}

	void SceneWindow::OnImGuiRender()
	{
		FLARE_PROFILE_FUNCTION();
		ImGui::Begin("Scene");

		if (m_Scene == nullptr)
		{
			ImGui::End();
			return;
		}

		World& world = m_Scene->GetECSWorld();

		Entity selected;
		if (EditorLayer::GetInstance().Selection.GetType() == EditorSelectionType::Entity)
			selected = EditorLayer::GetInstance().Selection.GetEntity();

		if (m_Hierarchy.OnRenderImGui(selected))
			EditorLayer::GetInstance().Selection.SetEntity(selected);

		ImGui::End();
	}

	void SceneWindow::Initialize(Ref<Scene> scene, const EditorCamera* editorCamera)
	{
		FLARE_PROFILE_FUNCTION();
		m_Scene = scene;

		if (m_Scene)
		{
			m_Hierarchy.SetWorld(m_Scene->GetECSWorld());
		}

		m_Hierarchy.SetEditorCamera(editorCamera);
	}

	void SceneWindow::Reset()
	{
		m_Scene = nullptr;
	}
}
