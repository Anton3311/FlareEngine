#include "PCH.h"

#include "EntitiesHierarchy.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Scene/Transform.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Hierarchy.h"
#include "Flare/Scene/Prefab.h"

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
		: m_World(nullptr), m_Features(features)
	{
		SetWorld(world);
	}

	bool EntitiesHierarchy::OnRenderImGui(Entity& selectedEntity)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_World);

		bool result = false;
		const std::vector<EntityRecord>& records = m_World->Entities.GetEntityRecords();

		ImGui::BeginChild("Scene Entities");

		if (ImGui::BeginPopupContextWindow("Entity Hierarchy Context Menu"))
		{
			result |= RenderContextMenu(selectedEntity, nullptr, true);
			ImGui::EndMenu();
		}

		if (m_CurrentEntityCount != m_World->Entities.GetEntityRecords().size())
		{
			BuildClippingAccelerationStructure();
			m_CurrentEntityCount = m_World->Entities.GetEntityRecords().size();
		}

		result |= RenderClippedHierarchy(selectedEntity);

		if (m_EntityToDelete)
		{
			HierarchyHelper::DeleteEntityHierarchy(*m_World, *m_EntityToDelete);
			m_EntityToDelete = {};

			result = true;
		}

		if (m_EntityToDuplicate)
		{
			FLARE_CORE_ASSERT(HAS_BIT(m_Features, EntitiesHierarchyFeatures::DuplicateEntity));

			// NOTE: Ignore SerializationId, because every entity should have a unique SerializationId
			std::unordered_set<ComponentId> ignoredComponents = { COMPONENT_ID(SerializationId) };
			selectedEntity = HierarchyHelper::DuplicateEntityHierarchy(*m_World, *m_EntityToDuplicate, &ignoredComponents);

			m_EntityToDuplicate = {};

			result = true;
		}

		ImGui::EndChild();

		return result;
	}

	void EntitiesHierarchy::SetWorld(World& world)
	{
		FLARE_PROFILE_FUNCTION();
		m_World = &world;

		m_RootLevelEntities = m_World->NewQuery().All().Without<Parent>().Build();

		BuildClippingAccelerationStructure();
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

	static bool IsTreeNodeOpened(ImGuiID id)
	{
		ImGuiContext* g = ImGui::GetCurrentContext();
		ImGuiStorage* storage = g->CurrentWindow->DC.StateStorage;
		return static_cast<bool>(storage->GetInt(id, 0));
	}

	bool EntitiesHierarchy::RenderEntityItem(Entity entity, Entity& selectedEntity, size_t accelerationStructureEntryIndex)
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

		bool isPrefab = m_World->HasComponent<PrefabInstance>(entity);

		if (!children)
			flags |= ImGuiTreeNodeFlags_Leaf;

		if (isPrefab)
			ImGui::PushStyleColor(ImGuiCol_Text, ImGuiTheme::Primary);

	 	void* id = (void*)std::hash<Entity>()(entity);
		ImGuiID nodeId = ImGui::GetCurrentWindow()->GetID(id);

		bool wasOpenedBefore = IsTreeNodeOpened(nodeId);

		bool opened = false;
		if (entityName)
			opened = ImGui::TreeNodeEx(id, flags, entityName->Value.c_str());
		else
			opened = ImGui::TreeNodeEx(id, flags, "Entity %d", entity.GetIndex());

		if (isPrefab)
			ImGui::PopStyleColor();

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

		if (children && accelerationStructureEntryIndex != SIZE_MAX && opened != wasOpenedBefore)
		{
			if (opened)
				m_ClippingAccelerationStruture[accelerationStructureEntryIndex].VisibleCount += children->GetChildren().size();
			else
				m_ClippingAccelerationStruture[accelerationStructureEntryIndex].VisibleCount -= children->GetChildren().size();
		}

		if (children && opened)
		{
			for (Entity child : children->GetChildren())
			{
				if (!m_World->IsEntityAlive(child))
					continue;

				result |= RenderEntityItem(child, selectedEntity, SIZE_MAX);
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
				FLARE_CORE_ASSERT(HAS_BIT(m_Features, EntitiesHierarchyFeatures::DuplicateEntity));

				m_EntityToDuplicate = entity;
			}

			ImGui::EndMenu();
		}

		return result;
	}

	bool EntitiesHierarchy::RenderClippedHierarchy(Entity& selectedEntity)
	{
		bool result = false;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		float itemHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y;
		float itemWidth = ImGui::GetContentRegionAvail().x;

		ImGuiWindow* window = ImGui::GetCurrentWindow();

		for (size_t i = 0; i < m_ClippingAccelerationStruture.size(); i++)
		{
			const auto& entry = m_ClippingAccelerationStruture[i];

			ImVec2 itemSize = ImVec2(itemWidth, itemHeight * static_cast<float>(entry.VisibleCount));
			ImVec2 rectMin = window->DC.CursorPos;
			ImVec2 rectMax = window->DC.CursorPos + itemSize;

			ImRect itemRect = ImRect(rectMin, rectMax);

			if (itemRect.Overlaps(window->ClipRect))
			{
				if (entry.Count > 1)
				{
					ImGuiListClipper clipper;
					clipper.Begin(static_cast<int32_t>(entry.VisibleCount), itemHeight);

					while (clipper.Step())
					{
						size_t visibleRangeStart = static_cast<size_t>(clipper.DisplayStart) + entry.Start;
						size_t visibleRangeEnd = static_cast<size_t>(clipper.DisplayEnd) + entry.Start;
						m_RootLevelEntities.ForEachEntityInRange(visibleRangeStart, visibleRangeEnd, [&](Entity entity)
							{
								result |= RenderEntityItem(entity, selectedEntity, i);
							});
					}
				}
				else
				{
					m_RootLevelEntities.ForEachEntityInRange(entry.Start, entry.Start + entry.Count, [&](Entity entity)
						{
							result |= RenderEntityItem(entity, selectedEntity, i);
						});
				}
			}
			else
			{
				ImGuiID id = window->GetID(&entry.CurrentEntity);
				ImGui::ItemSize(itemSize);
				ImGui::ItemAdd({ rectMin, rectMax }, id);
			}
		}

		ImGui::PopStyleVar();

		return result;
	}

	void EntitiesHierarchy::BuildClippingAccelerationStructure()
	{
		FLARE_PROFILE_FUNCTION();
		
		m_ClippingAccelerationStruture.clear();

		size_t offset = 0;
		m_RootLevelEntities.ForEachChunk([&](QueryChunk chunk)
			{
				for (size_t i = 0; i < chunk.GetEntityCount(); i++)
				{
					Entity entity = chunk.GetEntityId(i);

					if (m_ClippingAccelerationStruture.size() == 0)
					{
						m_ClippingAccelerationStruture.emplace_back(0, 0, 0, entity);
					}
					else
					{
						m_ClippingAccelerationStruture.back().Count++;
						m_ClippingAccelerationStruture.back().VisibleCount++;
						offset++;

						if (m_World->HasComponent<Children>(entity))
						{
							m_ClippingAccelerationStruture.emplace_back(offset, 0, 0, entity);
						}
					}
				}
			});
	}
}
