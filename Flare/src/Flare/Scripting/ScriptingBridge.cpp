#include "ScriptingBridge.h"

namespace Flare
{
    World* ScriptingBridge::s_World = nullptr;

    static Entity CreateEntity_Wrapper()
    {
        FLARE_CORE_INFO("Creating a new entity from a scripting module :)");
        return Entity();
    }

    void ScriptingBridge::ConfigureModule(Internal::ModuleConfiguration& config)
    {
        using namespace Internal;

        WorldBindings& worldBinding = *config.WorldBindings;

        worldBinding.CreateEntity = (WorldBindings::CreateEntityFunction)CreateEntity_Wrapper;
    }

    inline World& ScriptingBridge::GetCurrentWorld()
    {
        FLARE_CORE_ASSERT(s_World != nullptr);
        return *s_World;
    }
}
