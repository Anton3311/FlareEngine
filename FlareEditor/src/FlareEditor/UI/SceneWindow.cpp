#include "SceneWindow.h"

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Scene.h"

#include "FlareECS/World.h"
#include "FlareECS/Query/EntitiesIterator.h"
#include "FlareECS/Entities.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Flare
{
	void SceneWindow::OnImGuiRender()
	{
		ImGui::Begin("Scene");

		if (Scene::GetActive() == nullptr)
		{
			ImGui::End();
			return;
		}

		World& world = Scene::GetActive()->GetECSWorld();
		const auto& records = world.GetEntities().GetEntityRecords();

		ImGui::BeginChild("Scene Entities");
		ImGuiListClipper clipper;
		clipper.Begin((int32_t)records.size());

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
				ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
					| ImGuiTreeNodeFlags_FramePadding
					| ImGuiTreeNodeFlags_Bullet
					| ImGuiTreeNodeFlags_SpanFullWidth;

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding / 2);

				bool selected = entity == EditorLayer::GetInstance().Selection.TryGetEntity().value_or(Entity());
				if (selected)
				{
					flags |= ImGuiTreeNodeFlags_Framed;
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
					ImGui::PushStyleColor(ImGuiCol_Header, ImGuiTheme::Primary);
					ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGuiTheme::Primary);
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGuiTheme::PrimaryVariant);
				}

				bool opened = ImGui::TreeNodeEx((void*)std::hash<Entity>()(entity), flags, "Entity %d", entity.GetIndex());

				if (selected)
				{
					ImGui::PopStyleVar(1);
					ImGui::PopStyleColor(3);
				}

				ImGui::PopStyleVar(1);

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload(ENTITY_PAYLOAD_NAME, &entity, sizeof(Entity));
					ImGui::EndDragDropSource();
				}

				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
					EditorLayer::GetInstance().Selection.SetEntity(entity);

				if (opened)
					ImGui::TreePop();

				if (ImGui::BeginPopupContextItem())
				{
					if (ImGui::MenuItem("Delete"))
						world.DeleteEntity(entity);

					if (ImGui::MenuItem("Duplicate"))
					{
						Entities& entities = world.GetEntities();
						ArchetypeId archetype = entities.GetEntityArchetype(entity);
						Entity duplicated = entities.CreateEntityFromArchetype(archetype, ComponentInitializationStrategy::Zero);
						
						const ArchetypeRecord& record = world.GetArchetypes()[archetype];
						std::memcpy(entities.GetEntityData(duplicated).value(),
							entities.GetEntityData(entity).value(),
							entities.GetEntityStorage(archetype).GetEntitySize());
					}

					ImGui::EndMenu();
				}
			}
		}

		clipper.End();
		ImGui::EndChild();

		ImGui::End();
	}
}