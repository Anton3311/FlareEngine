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

	class Registry
	{
	public:
		Entity CreateEntity(ComponentSet& components);
		bool AddEntityComponent(Entity entity, ComponentId componentId, const void* componentData);

		EntityView View(ComponentSet components);
		std::optional<void*> GetEntityComponent(Entity entity, ComponentId component);

		inline ArchetypeRecord& GetArchetypeRecord(size_t archetypeId) { return m_Archetypes[archetypeId]; }

		ComponentId RegisterComponent(std::string_view name, size_t size);
		
		inline const ComponentInfo& GetComponentInfo(size_t index) const;
		inline const std::vector<ComponentInfo>& GetRegisteredComponents() const { return m_RegisteredComponents; }
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
	};
}