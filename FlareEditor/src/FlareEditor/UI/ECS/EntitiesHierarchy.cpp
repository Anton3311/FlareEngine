#include "PCH.h"

#include "EntitiesHierarchy.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Scene/Transform.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Hierarchy.h"

#include "FlareECS/World.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"

#include "FlareEditor/Serialization/SerializationId.h"

namespace Flare
{
	EntitiesHierarchy::EntitiesHierarchy(EntitiesHierarchyFeatures features)
		: m_World(nullptr), m_Features(features) {}

	EntitiesHierarchy::EntitiesHierarchy(World& world, EntitiesHierarchyFeatures features)
		: m_World(&world), m_Features(features) {}

	bool EntitiesHierarchy::OnRenderImGui(Entity& selectedEntity)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_World);

		bool result = false;
		const std::vector<EntityRecord>& records = m_World->Entities.GetEntityRecords();

#define USE_CLIPPER 0

		ImGui::BeginChild("Scene Entities");

#if USE_CLIPPER
		ImGuiListClipper clipper;
		clipper.Begin((int32_t)records.size());
#endif

		if (ImGui::BeginPopupContextWindow("Entity Hierarchy Context Menu"))
		{
			result |= RenderContextMenu(selectedEntity, nullptr, true);
			ImGui::EndMenu();
		}

#if USE_CLIPPER
		while (clipper.Step())
		{
			for (int32_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				if (i < 0 || i >= (int32_t)records.size())
					continue;

				Entity entity = records[i].Id;
				if (!m_World->HasComponent<Parent>(entity))
					result |= RenderEntityItem(entity, selectedEntity);
			}
		}

		clipper.End();
#else
		for (const EntityRecord& entityRecord : records)
		{
			if (!m_World->HasComponent<Parent>(entityRecord.Id))
				result |= RenderEntityItem(entityRecord.Id, selectedEntity);
		}

		if (m_EntityToDelete)
		{
			HierarchyHelper::DeleteEntityHierarchy(*m_World, *m_EntityToDelete);
			m_EntityToDelete = {};
		}
#endif

		ImGui::EndChild();

		return result;
	}

	bool EntitiesHierarchy::RenderContextMenu(Entity& selectedEntity, Entity* parent, bool isRoot)
	{
		FLARE_PROFILE_FUNCTION();

		bool result = false;

		bool isCreationSupported = HAS_BIT(m_Features, EntitiesHierarchyFeatures::CreateEntity);
		if (isRoot && !HAS_BIT(m_Features, EntitiesHierarchyFeatures::MultipleRootEntities))
			isCreationSupported = false;

		if (isCreationSupported)
		{
			if (ImGui::BeginMenu("Create"))
			{
				if (ImGui::MenuItem("Entity"))
				{
					selectedEntity = m_World->CreateEntity<TransformComponent, SerializationId>();
					result = true;
				}

				if (ImGui::MenuItem("Sprite"))
				{
					selectedEntity = m_World->CreateEntity<TransformComponent, SpriteComponent, SerializationId>();
					result = true;
				}

				if (ImGui::MenuItem("Perspective Camera"))
				{
					selectedEntity = m_World->CreateEntity(
						TransformComponent(),
						SerializationId(),
						CameraComponent(CameraComponent::ProjectionType::Perspective));
					result = true;
				}

				if (ImGui::MenuItem("Orthographic Camera"))
				{
					selectedEntity = m_World->CreateEntity(
						TransformComponent(),
						SerializationId(),
						CameraComponent(CameraComponent::ProjectionType::Orthographic));
					result = true;
				}

				if (ImGui::MenuItem("Directional Light"))
				{
					selectedEntity = m_World->CreateEntity(TransformComponent(), SerializationId(), DirectionalLight());
					result = true;
				}

				if (ImGui::MenuItem("Point Light"))
				{
					selectedEntity = m_World->CreateEntity(TransformComponent(), SerializationId(), PointLight());
					result = true;
				}

				if (ImGui::MenuItem("Spot Light"))
				{
					selectedEntity = m_World->CreateEntity(TransformComponent(), SerializationId(), SpotLight());
					result = true;
				}

				if (ImGui::MenuItem("Environment"))
				{
					selectedEntity = m_World->CreateEntity(TransformComponent(), SerializationId(), Environment());
					result = true;
				}

				if (result && parent)
				{
					HierarchyHelper::AddParent(*m_World, selectedEntity, *parent);
					m_World->AddEntityComponent(selectedEntity, LocalTransform());
				}

				ImGui::EndMenu();
			}
		}

		return result;
	}

	Entity EntitiesHierarchy::DuplicateEntity(Entity entity)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(HAS_BIT(m_Features, EntitiesHierarchyFeatures::DuplicateEntity));

		// TODO: Duplicate the whole hierarchy

		Entities& entities = m_World->Entities;
		ArchetypeId archetypeId = entities.GetEntityArchetype(entity);
		Entity duplicated = entities.CreateEntityFromArchetype(archetypeId, ComponentInitializationStrategy::DefaultConstructor);

		const ArchetypeRecord& archetype = m_World->GetArchetypes()[archetypeId];
		const ArchetypeComponents& archetypeComponents = m_World->GetArchetypes().GetArchetypeComponents(archetypeId);
		for (size_t i = 0; i < archetypeComponents.ComponentCount; i++)
		{
			if (archetypeComponents.ComponentIds[i] == COMPONENT_ID(SerializationId))
			{
				// NOTE: Skip SerializationId component, because each entity must have a unique serialization id.
				//
				//       Copying the id will result in problems when deserializing entity references,
				//       SceneSerializer might deserialize the wrong entity reference because there will
				//       be multiple entities with the same id
				//
				//       A unique id is generated by initializing components using a default constructor earlier
				continue;
			}

			void* componentSource = entities.GetEntityComponent(entity, archetypeComponents.ComponentIds[i]);
			void* componentDestination = entities.GetEntityComponent(duplicated, archetypeComponents.ComponentIds[i]);

			FLARE_CORE_ASSERT(componentSource && componentDestination);

			const ComponentInfo& component = m_World->Components.GetComponentInfo(archetypeComponents.ComponentIds[i]);
			component.Initializer->Type.Functions.CopyConstructor(componentDestination, componentSource);
		}

		return duplicated;
	}

	bool EntitiesHierarchy::RenderEntityItem(Entity entity, Entity& selectedEntity)
	{
		FLARE_PROFILE_FUNCTION();
		bool result = false;
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_SpanFullWidth;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImGui::GetStyle().FramePadding / 2);

		bool selected = entity == selectedEntity;
		if (selected)
			flags |= ImGuiTreeNodeFlags_Selected;

		NameComponent* entityName = m_World->TryGetEntityComponent<NameComponent>(entity);
		Children* children = m_World->TryGetEntityComponent<Children>(entity);

		if (!children)
			flags |= ImGuiTreeNodeFlags_Leaf;

		bool opened = false;
		if (entityName)
			opened = ImGui::TreeNodeEx((void*)std::hash<Entity>()(entity), flags, entityName->Value.c_str());
		else
			opened = ImGui::TreeNodeEx((void*)std::hash<Entity>()(entity), flags, "Entity %d", entity.GetIndex());

		result |= RenderEntityContextMenu(entity, selectedEntity);

		ImGui::PopStyleVar(1); // Frame padding

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

		if (children && opened)
		{
			for (Entity child : children->ChildrenEntities)
			{
				if (!m_World->IsEntityAlive(child))
					continue;

				result |= RenderEntityItem(child, selectedEntity);
			}
		}

		if (opened)
			ImGui::TreePop();

		return result;
	}

	bool EntitiesHierarchy::RenderEntityContextMenu(Entity entity, Entity& selectedEntity)
	{
		FLARE_PROFILE_FUNCTION();
		bool result = false;
		if (ImGui::BeginPopupContextItem())
		{
			RenderContextMenu(selectedEntity, &entity, false);

			if (HAS_BIT(m_Features, EntitiesHierarchyFeatures::DeleteEntity) && ImGui::MenuItem("Delete"))
			{
				m_EntityToDelete = entity;
			}

			if (HAS_BIT(m_Features, EntitiesHierarchyFeatures::DuplicateEntity) && ImGui::MenuItem("Duplicate"))
			{
				selectedEntity = DuplicateEntity(entity);
				result = true;
			}

			ImGui::EndMenu();
		}

		return result;
	}
}
