#include "ScriptingModule.h"

#include "FlareScripting/Platform/WindowsScriptingModule.h"

namespace Flare
{
    Ref<ScriptingModule> Flare::ScriptingModule::Create(const std::filesystem::path& path)
    {
#ifdef FLARE_PLATFORM_WINDOWS
        return CreateRef<WindowsScriptingModule>(path);
#endif
        return nullptr;
    }
}
