#include "Prefab.h"

#include "Flare/AssetManager/AssetManager.h"

#include <yaml-cpp/yaml.h>

namespace Flare
{
    Entity Prefab::CreateInstance(World& world)
    {
        Entity entity;
        if (m_Archetype == INVALID_ARCHETYPE_ID)
        {
            entity = world.Entities.CreateEntity(ComponentSet(m_Components), ComponentInitializationStrategy::Zero);
            m_Archetype = world.Entities.GetEntityArchetype(entity);
        }
        else
            entity = world.Entities.CreateEntityFromArchetype(m_Archetype, ComponentInitializationStrategy::Zero);

        auto data = world.Entities.GetEntityData(entity);
        size_t entitySize = world.Entities.GetEntityStorage(m_Archetype).GetEntitySize();

        if (data.has_value())
            std::memcpy(data.value(), m_Data, entitySize);

        return entity;
    }

    InstantiatePrefab::InstantiatePrefab(const Ref<Prefab>& prefab)
        : m_Prefab(prefab) {}

    void InstantiatePrefab::Apply(World& world)
    {
        m_Prefab->CreateInstance(world);
    }
}
