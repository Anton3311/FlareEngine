#include "ScriptingBridge.h"

namespace Flare
{
    World* ScriptingBridge::s_World = nullptr;

    static Entity CreateEntity_Wrapper()
    {
        FLARE_CORE_INFO("Creating a new entity from a scripting module :)");
        return Entity();
    }

    static size_t GetArchetypeComponentOffset_Wrapper(ArchetypeId archetype, ComponentId component)
    {
        Registry& registry = ScriptingBridge::GetCurrentWorld().GetRegistry();
        ArchetypeRecord& record = registry.GetArchetypeRecord(archetype);

        std::optional<size_t> index = registry.GetArchetypeComponentIndex(archetype, component);
        FLARE_CORE_ASSERT(index.has_value(), "Archetype doesn't have a component with given id");

        return record.Data.ComponentOffsets[index.value()];
    }

    void ScriptingBridge::ConfigureModule(Internal::ModuleConfiguration& config)
    {
        using namespace Internal;

        WorldBindings& worldBinding = *config.WorldBindings;
        EntityViewBindings& entityViewBindings = *config.EntityViewBindings;

        worldBinding.CreateEntity = (WorldBindings::CreateEntityFunction)CreateEntity_Wrapper;
        entityViewBindings.GetArchetypeComponentOffset = (EntityViewBindings::GetArchetypeComponentOffsetFunction)GetArchetypeComponentOffset_Wrapper;
    }

    inline World& ScriptingBridge::GetCurrentWorld()
    {
        FLARE_CORE_ASSERT(s_World != nullptr);
        return *s_World;
    }
}
