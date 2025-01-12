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
	//
	// EntitiesHierarchyTreeStructure
	//

	void EntitiesHierarchyAccelerationStructure::Build(const World& world, Query& rootLevelEntitiesQuery)
	{
		FLARE_PROFILE_FUNCTION();
		
		m_Nodes.clear();

		size_t offset = 0;
		size_t previousEntryIndex = SIZE_MAX;
		rootLevelEntitiesQuery.ForEachChunk([&](QueryChunk chunk)
			{
				for (size_t i = 0; i < chunk.GetEntityCount(); i++)
				{
					Entity entity = chunk.GetEntityId(i);
					bool hasChildren = world.HasComponent<Children>(entity);

					if (m_Nodes.size() == 0)
					{
						previousEntryIndex = AppendNode(entity, SIZE_MAX, 0, !hasChildren);
					}
					else
					{
						offset++;
						auto& previousNode = m_Nodes[previousEntryIndex];

						if (previousNode.IsLeaf && !hasChildren)
						{
							previousNode.VisibleCount++;
						}
						else
						{
							size_t node = AppendNode(entity, SIZE_MAX, offset, !hasChildren);

							previousNode.NextNode = node;
							previousEntryIndex = node;
						}
					}

					if (hasChildren)
					{
						BuildClippingSubStructure(world, entity, previousEntryIndex);
					}
				}
			});
	}

	void EntitiesHierarchyAccelerationStructure::BuildClippingSubStructure(const World& world, Entity rootEntity, size_t rootEntry)
	{
		FLARE_PROFILE_FUNCTION();

		const Children* children = world.TryGetEntityComponent<const Children>(rootEntity);
		FLARE_CORE_ASSERT(children);

		const auto& childrenEntities = children->GetChildren();
		size_t previousEntryIndex = SIZE_MAX;

		for (size_t i = 0; i < childrenEntities.size(); i++)
		{
			bool hasChildren = world.HasComponent<Children>(childrenEntities[i]);

			if (i == 0)
			{
				previousEntryIndex = AppendNode(childrenEntities[i], rootEntry, 0, !hasChildren);
			}
			else
			{
				auto& previousNode = m_Nodes[previousEntryIndex];

				if (previousNode.IsLeaf && !hasChildren)
				{
					previousNode.VisibleCount++;
				}
				else
				{
					size_t node = AppendNode(childrenEntities[i], rootEntry, i, !hasChildren);

					previousNode.NextNode = node;
					previousEntryIndex = node;
				}
			}

			if (hasChildren)
				BuildClippingSubStructure(world, childrenEntities[i], previousEntryIndex);
		}
	}

	size_t EntitiesHierarchyAccelerationStructure::AppendNode(Entity entity, size_t parentNode, size_t offset, bool isLeaf)
	{
		m_Nodes.push_back(Node
			{
				.Start = offset,
				.VisibleCount = 1,
				.NextNode = SIZE_MAX,
				.ParentNode = parentNode,
				.CurrentEntity = entity,
				.IsLeaf = isLeaf
			});

		return m_Nodes.size() - 1;
	}

	void EntitiesHierarchyAccelerationStructure::UpdateAncestorsVisibility(size_t startNode, int64_t visibleCountDelta)
	{
		FLARE_PROFILE_FUNCTION();

		size_t currentNode = startNode;
		while (currentNode != SIZE_MAX)
		{
			m_Nodes[currentNode].VisibleCount += visibleCountDelta;
			currentNode = m_Nodes[currentNode].ParentNode;
		}
	}

	size_t EntitiesHierarchyAccelerationStructure::CountEntriesInSameLevel(size_t startNode)
	{
		FLARE_PROFILE_FUNCTION();

		size_t result = 0;
		size_t currentNode = startNode;
		while (currentNode != SIZE_MAX)
		{
			result += m_Nodes[currentNode].VisibleCount;
			currentNode = m_Nodes[currentNode].NextNode;
		}

		return result;
	}

	//
	// EntitiesHierarchy
	//

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
			m_ClippingHierarchy.Build(*m_World, m_RootLevelEntities);
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

		m_ClippingHierarchy.Build(*m_World, m_RootLevelEntities);
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
			auto& currentNode = m_ClippingHierarchy.GetNode(accelerationStructureEntryIndex);
			size_t entitiesInNextLevel = m_ClippingHierarchy.CountEntriesInSameLevel(accelerationStructureEntryIndex + 1);

			if (opened)
			{
				m_ClippingHierarchy.UpdateAncestorsVisibility(accelerationStructureEntryIndex, static_cast<int64_t>(entitiesInNextLevel));
			}
			else
			{
				m_ClippingHierarchy.UpdateAncestorsVisibility(accelerationStructureEntryIndex, -static_cast<int64_t>(entitiesInNextLevel));
			}
		}

		if (children && opened)
		{
			const auto& childrenEntities = children->GetChildren();

			size_t childNode = accelerationStructureEntryIndex + 1;
			for (size_t i = 0; i < childrenEntities.size(); i++)
			{
				Entity child = childrenEntities[i];
				if (!m_World->IsEntityAlive(child))
					continue;

				result |= RenderEntityItem(child, selectedEntity, childNode);
				childNode = m_ClippingHierarchy.GetNode(childNode).NextNode;
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

	static bool RenderClippedTreeSection(const EntitiesHierarchyAccelerationStructure::Node& entry, float itemWidth, float itemHeight)
	{
		FLARE_PROFILE_FUNCTION();

		ImGuiWindow* window = ImGui::GetCurrentWindow();

		ImVec2 itemSize = ImVec2(itemWidth, itemHeight * static_cast<float>(entry.VisibleCount));
		ImVec2 rectMin = window->DC.CursorPos;
		ImVec2 rectMax = window->DC.CursorPos + itemSize;

		ImRect itemRect = ImRect(rectMin, rectMax);

		if (itemRect.Overlaps(window->ClipRect))
			return true;

		ImGuiID id = window->GetID(&entry.CurrentEntity);
		ImGui::ItemSize(itemSize);
		ImGui::ItemAdd({ rectMin, rectMax }, id);

		return false;
	}

	bool EntitiesHierarchy::RenderClippedHierarchy(Entity& selectedEntity)
	{
		FLARE_PROFILE_FUNCTION();
		bool result = false;

		const float itemHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y;
		const float itemWidth = ImGui::GetContentRegionAvail().x;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		size_t currentNode = 0;

		while (currentNode != SIZE_MAX)
		{
			const auto& node = m_ClippingHierarchy.GetNode(currentNode);

			if (RenderClippedTreeSection(node, itemWidth, itemHeight))
			{
				if (node.IsLeaf)
				{
					ImGuiListClipper clipper;
					clipper.Begin(static_cast<int32_t>(node.VisibleCount), itemHeight);

					while (clipper.Step())
					{
						size_t visibleRangeStart = static_cast<size_t>(clipper.DisplayStart) + node.Start;
						size_t visibleRangeEnd = static_cast<size_t>(clipper.DisplayEnd) + node.Start;
						m_RootLevelEntities.ForEachEntityInRange(visibleRangeStart, visibleRangeEnd, [&](Entity entity)
							{
								result |= RenderEntityItem(entity, selectedEntity, currentNode);
							});
					}
				}
				else
				{
					m_RootLevelEntities.ForEachEntityInRange(node.Start, node.Start + 1, [&](Entity entity)
						{
							result |= RenderEntityItem(entity, selectedEntity, currentNode);
						});
				}
			}

			currentNode = node.NextNode;
		}

		ImGui::PopStyleVar();

		return result;
	}
}
