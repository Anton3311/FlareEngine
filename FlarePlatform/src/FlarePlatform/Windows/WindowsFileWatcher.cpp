#include "WindowsFileWatcher.h"

#include "FlarePlatform/Windows/WindowsPlatform.h"

#include "FlareCore/Core.h"
#include "FlareCore/Log.h"

#include "FlarePlatform/Platform.h"

namespace Flare
{
    static constexpr size_t s_EventsBufferSize = 1024;

    WindowsFileWatcher::WindowsFileWatcher(const std::filesystem::path& directoryPath, EventsMask eventsMask)
        : m_DirectoryHandle(nullptr),
        m_CompletionKey(100),
        m_CompletionPort(nullptr),
        m_EventsBuffer(nullptr),
        m_Overlapped({}),
        m_EventsMask(eventsMask),
        m_EventsDataSize(0),
        m_NextEventOffset(0),
        m_IsValid(false)
    {
        if (!std::filesystem::exists(directoryPath))
        {
            FLARE_CORE_ERROR("FileWatcher: Invalid directory path '{}'", directoryPath.generic_string());
            return;
        }

        m_DirectoryHandle = CreateFileW(directoryPath.wstring().c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr);

        if (GetLastError())
        {
            FLARE_CORE_ERROR("FileWatcher: Failed to open '{}'", directoryPath.string());
            LogError();
            return;
        }

        m_CompletionPort = CreateIoCompletionPort(m_DirectoryHandle, nullptr, m_CompletionKey, 0);
        if (GetLastError())
        {
            FLARE_CORE_ERROR("FileWathcer: Failed to create a completion port");
            LogError();
            return;
        }

        ZeroMemory(&m_Overlapped, sizeof(m_Overlapped));

        m_EventsBuffer = new uint8_t[s_EventsBufferSize];
        m_IsValid = true;
    }

    WindowsFileWatcher::~WindowsFileWatcher()
    {
        if (m_CompletionPort != nullptr)
            CloseHandle(m_CompletionPort);
        if (m_DirectoryHandle != nullptr)
            CloseHandle(m_DirectoryHandle);

        if (m_EventsBuffer != nullptr)
            delete[] m_EventsBuffer;
    }

    bool WindowsFileWatcher::Update()
    {
        if (!m_IsValid)
            return false;

        DWORD notifyFilter = 0;
        if (HAS_BIT(m_EventsMask, EventsMask::DirectoryName))
            notifyFilter |= FILE_NOTIFY_CHANGE_DIR_NAME;
        if (HAS_BIT(m_EventsMask, EventsMask::FileName))
            notifyFilter |= FILE_NOTIFY_CHANGE_FILE_NAME;
        if (HAS_BIT(m_EventsMask, EventsMask::LastWrite))
            notifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE;

        DWORD bytesReturned;
        uint64_t completionKey;
        OVERLAPPED* overlapped;

        if (ReadDirectoryChangesW(
            m_DirectoryHandle,
            m_EventsBuffer,
            s_EventsBufferSize,
            true,
            notifyFilter,
            nullptr,
            &m_Overlapped,
            nullptr))
        {
            if (GetQueuedCompletionStatus(m_CompletionPort, &bytesReturned, &completionKey, &overlapped, 0))
            {
                m_EventsDataSize = (size_t)bytesReturned;
                m_NextEventOffset = 0;
            }
            else
            {
                constexpr DWORD timeOutErrorCode = 258;
                DWORD error = GetLastError();
                if (error != timeOutErrorCode)
                {
                    FLARE_CORE_ERROR("FileWatcher: Failed to get queued completion status");
                    LogError();
                    return false;
                }
            }

            return true;
        }
        else
        {
            FLARE_CORE_ERROR("FileWatcher: Failed to read directory changes");
            LogError();
            return false;
        }

        return false;
    }

    FileWatcher::Result WindowsFileWatcher::TryGetNextEvent(FileChangeEvent& outEvent)
    {
        if (!m_IsValid)
            return FileWatcher::Result::Error;

        bool stop = false;
        FileWatcher::Result result = FileWatcher::Result::NoEvents;
        while (m_NextEventOffset < m_EventsDataSize && !stop)
        {
            const FILE_NOTIFY_INFORMATION& notifyInfomation = *(const FILE_NOTIFY_INFORMATION*)(m_EventsBuffer + m_NextEventOffset);
            std::filesystem::path filePath = std::filesystem::path(std::wstring(notifyInfomation.FileName, notifyInfomation.FileNameLength / sizeof(wchar_t)));

            switch (notifyInfomation.Action)
            {
            case FILE_ACTION_ADDED:
                outEvent.Action = FileChangeEvent::ActionType::Created;
                outEvent.FilePath = std::move(filePath);
                stop = true;
                break;
            case FILE_ACTION_REMOVED:
                outEvent.Action = FileChangeEvent::ActionType::Deleted;
                outEvent.FilePath = std::move(filePath);
                stop = true;
                break;
            case FILE_ACTION_MODIFIED:
                outEvent.Action = FileChangeEvent::ActionType::Modified;
                outEvent.FilePath = std::move(filePath);
                stop = true;
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                outEvent.Action = FileChangeEvent::ActionType::Renamed;
                outEvent.OldFilePath = std::move(filePath);
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                outEvent.Action = FileChangeEvent::ActionType::Renamed;
                outEvent.FilePath = std::move(filePath);
                stop = true;
                break;
            }

            m_NextEventOffset += notifyInfomation.NextEntryOffset;
            result = FileWatcher::Result::Ok;

            if (notifyInfomation.NextEntryOffset == 0)
            {
                m_EventsDataSize = 0;
                m_NextEventOffset = 0;
                break;
            }
        }

        return result;
    }
}
