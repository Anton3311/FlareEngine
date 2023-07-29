#include "ScriptingBridge.h"

#include "Flare/Input/InputManager.h"

namespace Flare
{
    World* ScriptingBridge::s_World = nullptr;

    static Entity World_CreateEntity(const ComponentId* components, uint32_t count)
    {
        return ScriptingBridge::GetCurrentWorld().GetRegistry().CreateEntity(ComponentSet(components, (size_t)count));
    }

    static void* World_GetEntityComponent(Entity entity, ComponentId component)
    {
        return ScriptingBridge::GetCurrentWorld().GetRegistry().GetEntityComponent(entity, component).value_or(nullptr);
    }

    static std::optional<SystemGroupId> World_FindSystemGroup(std::string_view name)
    {
        return ScriptingBridge::GetCurrentWorld().GetSystemsManager().FindGroup(name);
    }

    static bool Input_IsKeyPressed(KeyCode key)
    {
        return InputManager::IsKeyPressed(key);
    }

    static bool Input_IsMouseButtonPreseed(MouseCode button)
    {
        return InputManager::IsMouseButtonPreseed(button);
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
        InputBindings& inputBindings = *config.InputBindings;

        worldBinding.CreateEntity = (WorldBindings::CreateEntityFunction)World_CreateEntity;
        worldBinding.FindSystemGroup = (WorldBindings::FindSystemGroupFunction)World_FindSystemGroup;
        worldBinding.GetEntityComponent = (WorldBindings::GetEntityComponentFunction)World_GetEntityComponent;

        entityViewBindings.GetArchetypeComponentOffset = (EntityViewBindings::GetArchetypeComponentOffsetFunction)GetArchetypeComponentOffset_Wrapper;

        inputBindings.IsKeyPressed = Input_IsKeyPressed;
        inputBindings.IsMouseButtonPressed = Input_IsMouseButtonPreseed;
    }

    inline World& ScriptingBridge::GetCurrentWorld()
    {
        FLARE_CORE_ASSERT(s_World != nullptr);
        return *s_World;
    }
}
