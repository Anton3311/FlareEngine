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
		ImGui::Begin("Scene");

		if (Scene::GetActive() == nullptr)
		{
			ImGui::End();
			return;
		}

		World& world = Scene::GetActive()->GetECSWorld();
		const auto& records = world.Entities.GetEntityRecords();

		m_Hierarchy.SetWorld(world);

		Entity selected;
		if (EditorLayer::GetInstance().Selection.GetType() == EditorSelectionType::Entity)
			selected = EditorLayer::GetInstance().Selection.GetEntity();

		if (m_Hierarchy.OnRenderImGui(selected))
			EditorLayer::GetInstance().Selection.SetEntity(selected);

		ImGui::End();
	}
}