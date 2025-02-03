#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/Query/Query.h"
#include "FlareECS/Commands/CommandBuffer.h"

namespace Flare
{
	class World;
	class EntitiesHierarchyAccelerationStructure
	{
	public:
		struct Node
		{
			size_t Start = 0;
			size_t VisibleCount = 0;
			size_t NextNode = SIZE_MAX;
			size_t ParentNode = SIZE_MAX;
			Entity CurrentEntity = Entity();
			bool IsLeaf = false;
		};

		EntitiesHierarchyAccelerationStructure() = default;

		void Build(const World& world, Query& rootLevelEntitiesQuery);
		void UpdateAncestorsVisibility(size_t startNode, int64_t visibleCountDelta);
		size_t CountEntriesInSameLevel(size_t startNode);

		inline const Node& GetNode(size_t index) const { return m_Nodes[index]; }
	private:
		void BuildClippingSubStructure(const World& world, Entity rootEntity, size_t rootEntry);
		size_t AppendNode(Entity entity, size_t parentNode, size_t offset, bool isLeaf);
	private:
		std::vector<Node> m_Nodes;
	};

	enum class EntitiesHierarchyFeatures
	{
		None = 0,
		CreateEntity = 1,
		DeleteEntity = 2,
		DuplicateEntity = 4,
		MultipleRootEntities = 8,

		All = CreateEntity | DeleteEntity | DuplicateEntity | MultipleRootEntities,
	};
	FLARE_IMPL_ENUM_BITFIELD(EntitiesHierarchyFeatures);

	class World;
	class EntitiesHierarchy
	{
	public:
		EntitiesHierarchy(EntitiesHierarchyFeatures features);
		EntitiesHierarchy(World& world, EntitiesHierarchyFeatures features);

		bool OnRenderImGui(Entity& selectedEntity);

		void SetFeatures(EntitiesHierarchyFeatures features) { m_Features = features; }

		void SetWorld(World& world);
	private:
		bool RenderContextMenu(Entity& selectedEntity, Entity* parent, bool isRoot);
		bool RenderEntityItem(Entity entity, Entity& selectedEntity, size_t accelerationStructureEntryIndex);
		bool RenderEntityContextMenu(Entity entity, Entity& selectedEntity);

		bool RenderClippedHierarchyRootLevel(Entity& selectedEntity);
	private:
		EntitiesHierarchyFeatures m_Features;

		std::optional<Entity> m_EntityToDelete;
		std::optional<Entity> m_EntityToDuplicate;

		EntitiesCommandBuffer m_EntityCommands;

		World* m_World = nullptr;

		Query m_RootLevelEntities;
		size_t m_CurrentEntityCount = 0;

		bool m_ClippingHierarchyIsDirty = false;
		EntitiesHierarchyAccelerationStructure m_ClippingHierarchy;
	};
}