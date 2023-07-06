#include "SceneWindow.h"

#include "Flare/Scene/Components.h"

#include "FlareECS/World.h"
#include "FlareECS/Query/EntityRegistryIterator.h"
#include "FlareECS/Registry.h"

#include "FlareEditor/EditorContext.h"

#include <imgui.h>

namespace Flare
{
	void SceneWindow::OnImGuiRender()
	{
		World& world = EditorContext::Instance.ActiveScene->GetECSWorld();

		ImGui::Begin("Scene");

		if (ImGui::BeginPopupContextWindow("Scene Context Menu"))
		{
			if (ImGui::MenuItem("Create Entity"))
				world.CreateEntity<TransformComponent>();

			ImGui::EndMenu();
		}

		std::optional<Entity> deletedEntity;
		for (Entity entity : world.GetRegistry())
		{
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding;

			bool opened = ImGui::TreeNodeEx((void*)std::hash<Entity>()(entity), flags, "Entity %d", entity.GetIndex());

			if (ImGui::IsItemClicked())
				EditorContext::Instance.SelectedEntity = entity;

			if (opened)
			{
				ImGui::TreePop();
			}

			if (ImGui::BeginPopupContextItem())
			{
				if (ImGui::MenuItem("Delete entity"))
					deletedEntity = entity;

				ImGui::EndMenu();
			}
		}

		if (deletedEntity.has_value())
			world.DeleteEntity(deletedEntity.value());

		ImGui::End();
	}
}