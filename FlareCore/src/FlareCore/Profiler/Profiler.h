#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include <stdint.h>
#include <chrono>

namespace Flare
{
    struct ProfilerScopeTimer;

    class FLARECORE_API Profiler
    {
    public:
        struct Record
        {
            const char* Name;
            uint64_t StartTime;
            uint64_t EndTime;
        };

        struct RecordsBuffer
        {
            RecordsBuffer(size_t capacity)
                : Records(new Record[capacity]), Size(0), Capacity(capacity) {}

            RecordsBuffer(RecordsBuffer&& other) noexcept
            {
                Records = other.Records;
                Size = other.Size;
                Capacity = other.Capacity;

                other.Records = nullptr;
                other.Size = 0;
                other.Capacity = 0;
            }

            ~RecordsBuffer()
            {
                delete[] Records;
            }

            inline bool IsEmpty() const { return Size == 0; }
            inline bool IsFull() const { return Size == Capacity; }

            Record* Records;
            size_t Capacity;
            size_t Size;
        };

        struct Frame
        {
            inline size_t GetRecordsCount() const { return RecordsCount - FirstRecordIndex; }

            size_t FirstRecordIndex;
            size_t RecordsCount;
        };

        static void BeginFrame();
        static void EndFrame();

        static void StartRecording();
        static void StopRecording();

        static size_t GetRecordsCountPerBuffer();
        static size_t GetRecordsCount();
        static Record& GetRecord(size_t index);

        static bool IsRecording();
        static size_t GetBuffersCount();
        static const RecordsBuffer& GetRecordsBuffer(size_t index);
        static const std::vector<Frame>& GetFrames();

        static void ClearData();
    private:
        static Record* CreateRecord();

        friend struct ProfilerScopeTimer;
    };

    struct ProfilerScopeTimer
    {
    public:
        ProfilerScopeTimer(const char* name)
        {
            m_Record = Profiler::CreateRecord();

            if (m_Record)
            {
                m_Record->Name = name;
                m_Record->StartTime = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
            }
        }

        ~ProfilerScopeTimer()
        {
            if (m_Record)
                m_Record->EndTime = (uint64_t) std::chrono::high_resolution_clock::now().time_since_epoch().count();
        }
    private:
        Profiler::Record* m_Record;
    };

#define FLARE_PROFILE_TIMER_NAME2(line) ___profileTimer___##line
#define FLARE_PROFILE_TIMER_NAME(line) FLARE_PROFILE_TIMER_NAME2(line)
#define FLARE_PROFILE_SCOPE(name) Flare::ProfilerScopeTimer FLARE_PROFILE_TIMER_NAME(__LINE__) = Flare::ProfilerScopeTimer(name)

#define FLARE_PROFILE_FUNCTION() FLARE_PROFILE_SCOPE(__FUNCSIG__)
}