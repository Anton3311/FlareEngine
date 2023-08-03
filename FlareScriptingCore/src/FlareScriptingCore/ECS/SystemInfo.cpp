#include "SystemInfo.h"

namespace Flare::Scripting
{
    std::vector<SystemInfo*>& SystemInfo::GetRegisteredSystems()
    {
        static std::vector<SystemInfo*> s_RegisteredSystems;
        return s_RegisteredSystems;
    }
}
