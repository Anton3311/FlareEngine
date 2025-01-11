#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/Entity/Entity.h"
#include "FlareECS/Query/Query.h"

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

		void SetFeatures(EntitiesHierarchyFeatures features) { m_Features = features; }

		void SetWorld(World& world);
	private:
		bool RenderContextMenu(Entity& selectedEntity, Entity* parent, bool isRoot);
		bool RenderEntityItem(Entity entity, Entity& selectedEntity);
		bool RenderEntityContextMenu(Entity entity, Entity& selectedEntity);

		void BuildClippingAccelerationStructure();
	private:
		EntitiesHierarchyFeatures m_Features;

		std::optional<Entity> m_EntityToDelete;
		std::optional<Entity> m_EntityToDuplicate;

		World* m_World;

		Query m_RootLevelEntities;
		size_t m_CurrentEntityCount;

		struct AccelerationStructureEntry
		{
			size_t Start = 0;
			size_t Count = 0;
			Entity CurrentEntity = Entity();
		};

		std::vector<AccelerationStructureEntry> m_ClippingAccelerationStruture;
	};
}