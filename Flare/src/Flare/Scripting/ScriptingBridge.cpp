#include "ScriptingBridge.h"

#include "Flare/Input/InputManager.h"

namespace Flare
{
    World* ScriptingBridge::s_World = nullptr;
    Scripting::Bindings ScriptingBridge::s_Bindings;

    static Entity World_CreateEntity(const ComponentId* components, size_t count)
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

    static bool Input_IsMouseButtonPressed(MouseCode button)
    {
        return InputManager::IsMouseButtonPreseed(button);
    }

    static size_t World_GetArchetypeComponentOffset(ArchetypeId archetype, ComponentId component)
    {
        Registry& registry = ScriptingBridge::GetCurrentWorld().GetRegistry();
        ArchetypeRecord& record = registry.GetArchetypeRecord(archetype);

        std::optional<size_t> index = registry.GetArchetypeComponentIndex(archetype, component);
        FLARE_CORE_ASSERT(index.has_value(), "Archetype doesn't have a component with given id");

        return record.Data.ComponentOffsets[index.value()];
    }



    void ScriptingBridge::Initialize()
    {
        using namespace Scripting;
        s_Bindings.CreateEntity = World_CreateEntity;
        s_Bindings.FindSystemGroup = World_FindSystemGroup;
        s_Bindings.GetEntityComponent = World_GetEntityComponent;

        s_Bindings.GetArchetypeComponentOffset = World_GetArchetypeComponentOffset;

        s_Bindings.IsKeyPressed = Input_IsKeyPressed;
        s_Bindings.IsMouseButtonPressed = Input_IsMouseButtonPressed;
    }

    inline World& ScriptingBridge::GetCurrentWorld()
    {
        FLARE_CORE_ASSERT(s_World != nullptr);
        return *s_World;
    }
}
