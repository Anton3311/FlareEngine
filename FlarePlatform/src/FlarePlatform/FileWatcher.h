#pragma once

#include "FlareCore/Core.h"

#include <filesystem>

namespace Flare
{
    struct FileChangeEvent
    {
        enum class ActionType
        {
            Invalid,
            Created,
            Deleted,
            Modified,
            Renamed,
        };

        std::filesystem::path FilePath;
        std::filesystem::path OldFilePath;
        ActionType Action;
    };

    enum class EventsMask
    {
        None = 0,
        FileName = 1,
        DirectoryName = 2,
        LastWrite = 4,
    };

    FLARE_IMPL_ENUM_BITFIELD(EventsMask);

    class FileWatcher
    {
    public:
        enum class Result
        {
            Ok,
            Error,
            NoEvents
        };
    public:
        virtual ~FileWatcher() {}

        virtual bool Update() = 0;
        virtual Result TryGetNextEvent(FileChangeEvent& outEvent) = 0;
    public:
        FLAREPLATFORM_API static FileWatcher* Create(const std::filesystem::path& directoryPath, EventsMask eventsMask);
    };
}
