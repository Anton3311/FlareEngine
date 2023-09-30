#include "EntitiesHierarchy.h"

#include "Flare/Scene/Components.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <imgui.h>

namespace Flare
{
	EntitiesHierarchy::EntitiesHierarchy(EntitiesHierarchyFeatures features)
		: m_World(nullptr), m_Features(features) {}

	EntitiesHierarchy::EntitiesHierarchy(World& world, EntitiesHierarchyFeatures features)
		: m_World(&world), m_Features(features) {}

	bool EntitiesHierarchy::OnRenderImGui(Entity& selectedEntity)
	{
		FLARE_CORE_ASSERT(m_World);

		bool result = false;
		const std::vector<EntityRecord>& records = m_World->Entities.GetEntityRecords();

		ImGui::BeginChild("Scene Entities");
		ImGuiListClipper clipper;
		clipper.Begin((int32_t)records.size());

		if (ImGui::BeginPopupContextWindow("Entity Hierarchy Context Menu"))
		{
			if (HAS_BIT(m_Features, EntitiesHierarchyFeatures::CreateEntity) && ImGui::MenuItem("Create Entity"))
				m_World->CreateEntity<TransformComponent>();

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

				bool selected = entity == selectedEntity;
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
				{
					selectedEntity = entity;
					result = true;
				}

				if (opened)
					ImGui::TreePop();

				if (ImGui::BeginPopupContextItem())
				{
					if (HAS_BIT(m_Features, EntitiesHierarchyFeatures::DeleteEntity) && ImGui::MenuItem("Delete"))
						m_World->DeleteEntity(entity);

					if (HAS_BIT(m_Features, EntitiesHierarchyFeatures::DuplicateEntity) && ImGui::MenuItem("Duplicate"))
					{
						Entities& entities = m_World->Entities;
						ArchetypeId archetype = entities.GetEntityArchetype(entity);
						Entity duplicated = entities.CreateEntityFromArchetype(archetype, ComponentInitializationStrategy::Zero);

						const ArchetypeRecord& record = m_World->GetArchetypes()[archetype];
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

		return result;
	}
}
