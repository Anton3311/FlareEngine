#pragma once

#include "FlarePlatform/FileWatcher.h"

#include <filesystem>
#include <windows.h>

namespace Flare
{
    class WindowsFileWatcher : public FileWatcher
    {
    public:
        WindowsFileWatcher(const std::filesystem::path& directoryPath, EventsMask eventsMask);
        virtual ~WindowsFileWatcher();

        bool Update() override;
        virtual FileWatcher::Result TryGetNextEvent(FileChangeEvent& outEvent) override;
    public:
        bool m_IsValid;

        HANDLE m_DirectoryHandle;
        HANDLE m_CompletionPort;

        uint8_t* m_EventsBuffer;
        uint64_t m_CompletionKey;

        OVERLAPPED m_Overlapped;
        EventsMask m_EventsMask;

        size_t m_NextEventOffset;
        size_t m_EventsDataSize;
    };
}
