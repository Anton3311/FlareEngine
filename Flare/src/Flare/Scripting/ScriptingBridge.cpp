#include "ScriptingBridge.h"

#include "Flare/Input/InputManager.h"

#include "FlareScriptingCore/ECS/EntityView.h"

#include <functional>

namespace Flare
{
    World* ScriptingBridge::s_World = nullptr;
    Scripting::Bindings ScriptingBridge::s_Bindings;

    // ECS

    static Entity World_CreateEntity(const ComponentId* components, size_t count)
    {
        return ScriptingBridge::GetCurrentWorld().GetRegistry().CreateEntity(ComponentSet(components, (size_t)count));
    }

    static bool World_IsEntityAlive(Entity entity)
    {
        return ScriptingBridge::GetCurrentWorld().IsEntityAlive(entity);
    }

    static void* World_GetEntityComponent(Entity entity, ComponentId component)
    {
        return ScriptingBridge::GetCurrentWorld().GetRegistry().GetEntityComponent(entity, component).value_or(nullptr);
    }

    static Entity World_GetSingletonEntity(QueryId queryId)
    {
        World& world = ScriptingBridge::GetCurrentWorld();
        Query query = Query(queryId, world.GetRegistry());

        return world.GetRegistry().GetSingletonEntity(query).value_or(Entity());
    }

    static void* World_GetSingletonComponent(ComponentId id)
    {
        World& world = ScriptingBridge::GetCurrentWorld();
        return world.GetRegistry().GetSingleComponent(id).value_or(nullptr);
    }

    static std::optional<SystemGroupId> World_FindSystemGroup(std::string_view name)
    {
        return ScriptingBridge::GetCurrentWorld().GetSystemsManager().FindGroup(name);
    }

    static size_t World_GetArchetypeComponentOffset(ArchetypeId archetype, ComponentId component)
    {
        Registry& registry = ScriptingBridge::GetCurrentWorld().GetRegistry();
        ArchetypeRecord& record = registry.GetArchetypeRecord(archetype);

        std::optional<size_t> index = registry.GetArchetypeComponentIndex(archetype, component);
        FLARE_CORE_ASSERT(index.has_value(), "Archetype doesn't have a component with given id");

        return record.Data.ComponentOffsets[index.value()];
    }

    static QueryId World_CreateQuery(const ComponentId* components, size_t count)
    {
        World& world = ScriptingBridge::GetCurrentWorld();
        return world.GetRegistry().CreateQuery(ComponentSet(components, count)).GetId();
    }

    static void World_ForEachChunkInQuery(QueryId queryId, const std::function<void(Scripting::EntityView& view)>& perChunk)
    {
        World& world = ScriptingBridge::GetCurrentWorld();
        Query query = Query(queryId, world.GetRegistry());

        for (EntityView view : query)
        {
            ArchetypeRecord& record = world.GetRegistry().GetArchetypeRecord(view.GetArchetype());

            size_t chunksCount = record.Storage.GetChunksCount();
            for (size_t i = 0; i < chunksCount; i++)
            {
                Scripting::EntityView chunk(
                    view.GetArchetype(),
                    record.Storage.GetChunkBuffer(i),
                    record.Storage.GetEntitySize(),
                    record.Storage.GetEntitiesCountInChunk(i));

                perChunk(chunk);
            }
        }
    }

    // Input

    static bool Input_IsKeyPressed(KeyCode key)
    {
        return InputManager::IsKeyPressed(key);
    }

    static bool Input_IsMouseButtonPressed(MouseCode button)
    {
        return InputManager::IsMouseButtonPreseed(button);
    }

    void ScriptingBridge::Initialize()
    {
        using namespace Scripting;
        s_Bindings.CreateEntity = World_CreateEntity;
        s_Bindings.IsEntityAlive = World_IsEntityAlive;
        s_Bindings.FindSystemGroup = World_FindSystemGroup;
        s_Bindings.GetEntityComponent = World_GetEntityComponent;

        s_Bindings.CreateQuery = World_CreateQuery;
        s_Bindings.ForEachChunkInQuery = World_ForEachChunkInQuery;

        s_Bindings.GetSingletonEntity = World_GetSingletonEntity;
        s_Bindings.GetSingletonComponent = World_GetSingletonComponent;

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
