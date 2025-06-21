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
		inline size_t GetNodeCount() const { return m_Nodes.size(); }
		inline bool IsEmpty() const { return m_Nodes.size() == 0; }
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
	class EditorCamera;
	class EntitiesHierarchy
	{
	public:
		EntitiesHierarchy(EntitiesHierarchyFeatures features);
		EntitiesHierarchy(World& world, EntitiesHierarchyFeatures features);

		bool OnRenderImGui(std::optional<Entity>& selectedEntity);

		void SetFeatures(EntitiesHierarchyFeatures features) { m_Features = features; }

		void SetWorld(World& world);
		inline void SetEditorCamera(const EditorCamera* editorCamera) { m_EditorCamera = editorCamera; }
	private:
		void RenderCreateEntityMenu(std::optional<Entity> parent, std::optional<Entity>& selectedEntity, bool isRoot);
		void RenderEntityItem(Entity entity, std::optional<Entity>& selectedEntity, size_t accelerationStructureEntryIndex);
		void RenderEntityContextMenu(Entity entity, std::optional<Entity>& selectedEntity);

		void RenderClippedHierarchyRootLevel(std::optional<Entity>& selectedEntity);
	private:
		EntitiesHierarchyFeatures m_Features;
		const EditorCamera* m_EditorCamera = nullptr;

		EntitiesCommandBuffer m_EntityCommands;

		World* m_World = nullptr;

		Query m_RootLevelEntities;
		size_t m_CurrentEntityCount = 0;

		bool m_ClippingHierarchyIsDirty = false;
		EntitiesHierarchyAccelerationStructure m_ClippingHierarchy;
	};
}
