#pragma once

#include "Flare/Core/Assert.h"
#include "Flare/Core/Core.h"

#include "FlareECS/Entity.h"
#include "FlareECS/Component.h"
#include "FlareECS/Archetype.h"

#include <unordered_map>
#include <vector>
#include <optional>

namespace Flare
{
	class EntityView;
	class EntityRegistryIterator;

	class Registry
	{
	public:
		Entity CreateEntity(ComponentSet& components);
		bool AddEntityComponent(Entity entity, ComponentId componentId, const void* componentData);

		EntityView View(ComponentSet components);
		const ComponentSet& GetEntityComponents(Entity entity);

		std::optional<void*> GetEntityComponent(Entity entity, ComponentId component);

		ComponentId RegisterComponent(std::string_view name, size_t size);

		inline ArchetypeRecord& GetArchetypeRecord(size_t archetypeId) { return m_Archetypes[archetypeId]; }
		
		inline const ComponentInfo& GetComponentInfo(size_t index) const;
		inline const std::vector<ComponentInfo>& GetRegisteredComponents() const { return m_RegisteredComponents; }

		EntityRegistryIterator begin();
		EntityRegistryIterator end();
	public:
		inline EntityRecord& operator[](size_t index);
		inline const EntityRecord& operator[](size_t index) const;
	private:
		ArchetypeId CreateArchetype();

		void RemoveEntityData(ArchetypeId archetype, size_t entityBufferIndex);
	private:
		std::vector<EntityRecord> m_EntityRecords;
		std::unordered_map<Entity, size_t> m_EntityToRecord;

		std::vector<ArchetypeRecord> m_Archetypes;
		std::unordered_map<ComponentSet, size_t> m_ComponentSetToArchetype;

		std::vector<ComponentInfo> m_RegisteredComponents;

		friend class EntityRegistryIterator;
	};
}