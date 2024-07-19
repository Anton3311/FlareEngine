#include "Prefab.h"

#include "Flare/AssetManager/AssetManager.h"

#include <yaml-cpp/yaml.h>

namespace Flare
{
    FLARE_IMPL_ASSET(Prefab);
    FLARE_SERIALIZABLE_IMPL(Prefab);

	Prefab::Prefab(const uint8_t* prefabData, const Components* compatibleComponentsRegistry, std::vector<std::pair<ComponentId, void*>>&& components)
		: Asset(AssetType::Prefab), m_Data(prefabData), m_Components(std::move(components)), m_CompatibleComponentsRegistry(compatibleComponentsRegistry)
    {
    }

    Prefab::~Prefab()
    {
        if (m_Data != nullptr)
        {
			for (const auto& [id, data] : m_Components)
			{
				auto& info = m_CompatibleComponentsRegistry->GetComponentInfo(id);
				info.Deleter(data);
			}

            delete[] m_Data;
        }
    }

    Entity Prefab::CreateInstance(World& world)
    {
        FLARE_CORE_ASSERT(&world.Components == m_CompatibleComponentsRegistry);
        return world.Entities.CreateEntity(m_Components.data(), m_Components.size(), true);
    }

    InstantiatePrefab::InstantiatePrefab(const Ref<Prefab>& prefab)
        : m_Prefab(prefab) {}

    void InstantiatePrefab::Apply(CommandContext& context, World& world)
    {
        Entity entity = m_Prefab->CreateInstance(world);
        context.SetEntity(m_OutputEntity, entity);
    }

    void InstantiatePrefab::Initialize(FutureEntity entity)
    {
        m_OutputEntity = entity;
    }
}
