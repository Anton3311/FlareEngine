#include "SceneWindow.h"

#include "Flare/Scene/Components.h"

#include "FlareECS/World.h"
#include "FlareECS/Query/EntityRegistryIterator.h"
#include "FlareECS/Registry.h"

#include "FlareEditor/EditorContext.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Flare
{
	void SceneWindow::OnImGuiRender()
	{
		World& world = EditorContext::GetActiveScene()->GetECSWorld();

		ImGui::Begin("Scene");

		const auto& records = world.GetRegistry().GetEntityRecords();

		ImGui::BeginChild("Scene Entities");
		ImGuiListClipper clipper;
		clipper.Begin(records.size());

		if (ImGui::BeginPopupContextWindow("Scene Context Menu"))
		{
			if (ImGui::MenuItem("Create Entity"))
				world.CreateEntity<TransformComponent>();

			ImGui::EndMenu();
		}

		std::optional<Entity> deletedEntity;
		while (clipper.Step())
		{
			for (int32_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				if (i < 0 || i >= (int32_t)records.size())
					continue;

				Entity entity = records[i].Id;
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
		}

		clipper.End();
		ImGui::EndChild();

		if (deletedEntity.has_value())
			world.DeleteEntity(deletedEntity.value());

		ImGui::End();
	}
}