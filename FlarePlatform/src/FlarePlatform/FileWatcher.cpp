#include "FileWatcher.h"

#include "FlarePlatform/Windows/WindowsFileWatcher.h"

namespace Flare
{
    FileWatcher* FileWatcher::Create(const std::filesystem::path& directoryPath, EventsMask eventsMask)
    {
#ifdef FLARE_PLATFORM_WINDOWS
        return new WindowsFileWatcher(directoryPath, eventsMask);
#endif
        return nullptr;
    }
}
