#include "SystemInfo.h"

namespace Flare
{
    std::vector<const SystemInfo*>& Flare::SystemInfo::GetRegisteredSystems()
    {
        static std::vector<const SystemInfo*> s_RegisteredSystems;
        return s_RegisteredSystems;
    }
}
