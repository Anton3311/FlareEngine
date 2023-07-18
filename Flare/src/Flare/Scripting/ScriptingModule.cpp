#include "ScriptingModule.h"

#include "Flare/Platform/Windows/WindowsScriptingModule.h"

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
