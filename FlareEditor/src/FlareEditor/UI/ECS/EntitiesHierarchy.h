#pragma once

#include "FlareCore/Core.h"

#include "FlareECS/World.h"

namespace Flare
{
	enum class EntitiesHierarchyFeatures
	{
		None = 0,
		CreateEntity = 1,
		DeleteEntity = 2,
		DuplicateEntity = 4,

		All = CreateEntity | DeleteEntity | DuplicateEntity,
	};
	FLARE_IMPL_ENUM_BITFIELD(EntitiesHierarchyFeatures);

	class EntitiesHierarchy
	{
	public:
		EntitiesHierarchy(EntitiesHierarchyFeatures features = EntitiesHierarchyFeatures::All);
		EntitiesHierarchy(World& world, EntitiesHierarchyFeatures features = EntitiesHierarchyFeatures::All);

		bool OnRenderImGui(Entity& selectedEntity);

		inline void SetWorld(World& world) { m_World = &world; }
	private:
		EntitiesHierarchyFeatures m_Features;
		World* m_World;
	};
}