#include "PCH.h"

#include "EntitiesHierarchy.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Scene/Transform.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Hierarchy.h"
#include "Flare/Scene/HierarchyCommands.h"
#include "Flare/Scene/Prefab.h"

#include "FlareECS/World.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareEditor/EditorCamera.h"

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
		rootLevelEntitiesQuery.ForEachChunk([&](QueryChunk chunk, ArchetypeId archetypeId)
			{
				bool hasChildren = world
					.GetArchetypes()
					.GetArchetypeComponents(archetypeId)
					.TryGetComponentIndex(COMPONENT_ID(Children)) != ArchetypeComponents::INVALID_COMPONENT_INDEX;
				
				for (size_t i = 0; i < chunk.GetEntityCount(); i++)
				{
					Entity entity = chunk.GetEntityId(i);

					if (m_Nodes.size() == 0)
					{
						previousEntryIndex = AppendNode(entity, SIZE_MAX, 0, !hasChildren);
					}
					else
					{
						offset++;

						if (m_Nodes[previousEntryIndex].IsLeaf && !hasChildren)
						{
							m_Nodes[previousEntryIndex].VisibleCount++;
						}
						else
						{
							size_t node = AppendNode(entity, SIZE_MAX, offset, !hasChildren);

							m_Nodes[previousEntryIndex].NextNode = node;
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
				if (m_Nodes[previousEntryIndex].IsLeaf && !hasChildren)
				{
					m_Nodes[previousEntryIndex].VisibleCount++;
				}
				else
				{
					size_t node = AppendNode(childrenEntities[i], rootEntry, i, !hasChildren);

					m_Nodes[previousEntryIndex].NextNode = node;
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

	static bool IsTreeNodeOpened(ImGuiID id)
	{
		FLARE_PROFILE_FUNCTION();
		ImGuiContext* g = ImGui::GetCurrentContext();
		ImGuiStorage* storage = g->CurrentWindow->DC.StateStorage;
		return static_cast<bool>(storage->GetInt(id, 0));
	}

	static void* GetEntityTreeNodeId(Entity entity)
	{
		return reinterpret_cast<void*>(std::hash<Entity>()(entity));
	}

	bool EntitiesHierarchy::OnRenderImGui(std::optional<Entity>& selectedEntity)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_World);

		const std::vector<EntityRecord>& records = m_World->Entities.GetEntityRecords();

		ImGui::BeginChild("Scene Entities");

		std::optional<Entity> newEntitySelection = selectedEntity;

		if (ImGui::BeginPopupContextWindow("Entity Hierarchy Context Menu"))
		{
			RenderCreateEntityMenu({}, newEntitySelection, true);
			ImGui::EndMenu();
		}

		if (auto storage = m_World->Events.TryGetEventStorage(COMPONENT_ID(ReparentEvent)))
		{
			if (storage->GetEventCount() != 0)
			{
				m_ClippingHierarchyIsDirty = true;
			}
		}

		if (m_CurrentEntityCount != m_World->Entities.GetEntityRecords().size())
		{
			m_ClippingHierarchyIsDirty = true;
		}

		if (m_ClippingHierarchyIsDirty)
		{
			m_ClippingHierarchy.Build(*m_World, m_RootLevelEntities);
			m_CurrentEntityCount = m_World->Entities.GetEntityRecords().size();
			m_ClippingHierarchyIsDirty = false;
		}

		RenderClippedHierarchyRootLevel(newEntitySelection);

		// NOTE: Must be executed at the end of imgui rendering
		if (m_EntityCommands.Execute(*m_World) > 0)
		{
			m_ClippingHierarchyIsDirty = true;
		}

		ImGui::EndChild();

		bool changed = newEntitySelection != selectedEntity;
		selectedEntity = newEntitySelection;
		return changed;
	}

	void EntitiesHierarchy::SetWorld(World& world)
	{
		FLARE_PROFILE_FUNCTION();
		m_World = &world;

		m_RootLevelEntities = m_World->NewQuery().All().Without<Parent>().Build();

		m_ClippingHierarchy.Build(*m_World, m_RootLevelEntities);
	}

	void EntitiesHierarchy::RenderCreateEntityMenu(std::optional<Entity> parent, std::optional<Entity>& selectedEntity, bool isRoot)
	{
		FLARE_PROFILE_FUNCTION();

		bool isCreationSupported = HAS_BIT(m_Features, EntitiesHierarchyFeatures::CreateEntity);
		if (isRoot && !HAS_BIT(m_Features, EntitiesHierarchyFeatures::MultipleRootEntities))
			isCreationSupported = false;

		auto setCreatedEntity = [this, parent, &selectedEntity](FutureEntity entity)
		{
			FutureEntityCommands commands = FutureEntityCommands(entity, m_EntityCommands);
			commands.ExecuteFunction([&selectedEntity](CommandContext& context, World& world, Entity entity)
			{
				selectedEntity = entity;
			});

			if (parent)
			{
				FutureEntity parentEntity = m_EntityCommands.GetEntity(*parent).GetFutureEntity();
				m_EntityCommands.AddCommand(SetParentCommand(entity, parentEntity));
				commands.AddComponent<LocalTransform>();
			}
		};

		if (!isCreationSupported)
			return;

		if (ImGui::BeginMenu("Create"))
		{
			if (ImGui::MenuItem("Entity"))
			{
				setCreatedEntity(m_EntityCommands.CreateEntity<TransformComponent, SerializationId>().GetFutureEntity());
			}

			if (ImGui::MenuItem("Sprite"))
			{
				setCreatedEntity(m_EntityCommands.CreateEntity<TransformComponent, SpriteComponent, SerializationId>().GetFutureEntity());
			}

			if (ImGui::MenuItem("Perspective Camera"))
			{
				setCreatedEntity(m_EntityCommands
					.CreateEntity(
						TransformComponent(),
						SerializationId(),
						CameraComponent(CameraComponent::ProjectionType::Perspective))
					.GetFutureEntity());
			}

			if (ImGui::MenuItem("Orthographic Camera"))
			{
				setCreatedEntity(m_EntityCommands
					.CreateEntity(
						TransformComponent(),
						SerializationId(),
						CameraComponent(CameraComponent::ProjectionType::Orthographic))
					.GetFutureEntity());
			}

			if (ImGui::MenuItem("Directional Light"))
			{
				setCreatedEntity(m_EntityCommands
					.CreateEntity(
						TransformComponent(),
						SerializationId(),
						DirectionalLight())
					.GetFutureEntity());
			}

			if (ImGui::MenuItem("Point Light"))
			{
				setCreatedEntity(m_EntityCommands
					.CreateEntity(
						TransformComponent(),
						SerializationId(),
						PointLight())
					.GetFutureEntity());
			}

			if (ImGui::MenuItem("Spot Light"))
			{
				setCreatedEntity(m_EntityCommands
					.CreateEntity(
						TransformComponent(),
						SerializationId(),
						SpotLight())
					.GetFutureEntity());
			}

			if (ImGui::MenuItem("Environment"))
			{
				setCreatedEntity(m_EntityCommands
					.CreateEntity(
						TransformComponent(),
						SerializationId(),
						Environment())
					.GetFutureEntity());
			}

			ImGui::EndMenu();
		}
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

	template<typename F>
	static void RenderClippedHierarchy(EntitiesHierarchyAccelerationStructure& clippingHierarchy, size_t startNode, F&& renderFunction)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(startNode < clippingHierarchy.GetNodeCount());

		const float itemHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y;
		const float itemWidth = ImGui::GetContentRegionAvail().x;

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

		size_t currentNode = startNode;

		while (currentNode != SIZE_MAX)
		{
			const auto& node = clippingHierarchy.GetNode(currentNode);

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

						renderFunction(visibleRangeStart, visibleRangeEnd, currentNode);
					}
				}
				else
				{
					renderFunction(node.Start, node.Start + 1, currentNode);
				}
			}

			currentNode = node.NextNode;
		}

		ImGui::PopStyleVar();
	}

	void EntitiesHierarchy::RenderEntityItem(Entity entity, std::optional<Entity>& selectedEntity, size_t accelerationStructureEntryIndex)
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

	 	void* id = GetEntityTreeNodeId(entity);
		ImGuiID nodeId = ImGui::GetCurrentWindow()->GetID(id);

		bool wasOpenedBefore = IsTreeNodeOpened(nodeId);

		bool opened = false;
		if (entityName)
			opened = ImGui::TreeNodeEx(id, flags, entityName->Value.c_str());
		else
			opened = ImGui::TreeNodeEx(id, flags, "Entity %d", entity.GetIndex());

		if (isPrefab)
			ImGui::PopStyleColor();

		RenderEntityContextMenu(entity, selectedEntity);

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
			size_t firstChildNode = accelerationStructureEntryIndex + 1;
			RenderClippedHierarchy(m_ClippingHierarchy, firstChildNode, [this, &selectedEntity, &children](size_t start, size_t end, size_t currentNode)
			{
				const auto& childrenEntities = children->GetChildren();
				for (size_t i = start; i < end; i++)
				{
					Entity child = childrenEntities[i];
					if (!m_World->IsEntityAlive(child))
						continue;

					RenderEntityItem(child, selectedEntity, currentNode);
				}
			});
		}

		if (opened)
			ImGui::TreePop();
	}

	void EntitiesHierarchy::RenderEntityContextMenu(Entity entity, std::optional<Entity>& selectedEntity)
	{
		FLARE_PROFILE_FUNCTION();
		if (ImGui::BeginPopupContextItem())
		{
			if (entity != selectedEntity)
			{
				selectedEntity = entity;
			}

			RenderCreateEntityMenu(entity, selectedEntity, false);

			if (HAS_BIT(m_Features, EntitiesHierarchyFeatures::DeleteEntity) && ImGui::MenuItem("Delete"))
			{
				m_EntityCommands.GetEntity(entity).ExecuteFunction([this, &selectedEntity](CommandContext& context, World& world, Entity entity)
				{
					HierarchyHelper::DeleteEntityHierarchy(world, entity);
					selectedEntity = {};
				});
			}

			if (HAS_BIT(m_Features, EntitiesHierarchyFeatures::DuplicateEntity) && ImGui::MenuItem("Duplicate"))
			{
				FLARE_CORE_ASSERT(HAS_BIT(m_Features, EntitiesHierarchyFeatures::DuplicateEntity));

				m_EntityCommands.GetEntity(entity).ExecuteFunction([this, &selectedEntity](CommandContext& context, World& world, Entity entity)
				{
					// NOTE: Ignore SerializationId, because every entity should have a unique SerializationId
					std::unordered_set<ComponentId> ignoredComponents = { COMPONENT_ID(SerializationId) };
					Entity duplicateEntity = HierarchyHelper::DuplicateEntityHierarchy(world, entity, &ignoredComponents);

					selectedEntity = duplicateEntity;
				});
			}

			if (m_World->HasComponent<Parent>(entity) && ImGui::MenuItem("Detach from parent"))
			{
				FutureEntity futureEntity = m_EntityCommands.GetEntity(entity).GetFutureEntity();
				m_EntityCommands.AddCommand(DetachFromParentCommand(futureEntity));
			}

			if (m_EditorCamera)
			{
				TransformComponent* globalTransform = m_World->TryGetEntityComponent<TransformComponent>(entity);
				LocalTransform* localTransform = m_World->TryGetEntityComponent<LocalTransform>(entity);

				bool hasTransform = globalTransform || localTransform;
				bool hasCamera = m_World->HasComponent<CameraComponent>(entity);

				if (hasTransform && hasCamera && ImGui::MenuItem("Match with editor camera"))
				{
					glm::vec3 position = m_EditorCamera->GetPosition();
					glm::vec3 rotation = m_EditorCamera->GetRotation();

					if (localTransform)
					{
						localTransform->Position = position;
						localTransform->Rotation = -rotation;
					}

					if (globalTransform)
					{
						globalTransform->Position = position;
						globalTransform->Rotation = -rotation;
					}
				}
			}


			ImGui::EndMenu();
		}
	}

	void EntitiesHierarchy::RenderClippedHierarchyRootLevel(std::optional<Entity>& selectedEntity)
	{
		FLARE_PROFILE_FUNCTION();

		if (m_ClippingHierarchy.IsEmpty())
		{
			return;
		}

		RenderClippedHierarchy(m_ClippingHierarchy, 0, [&](size_t start, size_t end, size_t currentNode)
		{
			bool result = false;
			m_RootLevelEntities.ForEachEntityInRange(start, end, [&](Entity entity)
			{
				RenderEntityItem(entity, selectedEntity, currentNode);
			});

			return result;
		});
	}
}
