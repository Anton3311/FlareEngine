#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
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

		inline void SetWorld(World& world) { m_World = &world; }
	private:
		bool RenderContextMenu(Entity& selectedEntity, Entity* parent, bool isRoot);
	private:
		bool RenderEntityItem(Entity entity, Entity& selectedEntity);
		bool RenderEntityContextMenu(Entity entity, Entity& selectedEntity);
	private:
		EntitiesHierarchyFeatures m_Features;

		std::optional<Entity> m_EntityToDelete;
		std::optional<Entity> m_EntityToDuplicate;

		World* m_World;
	};
}