#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include <stdint.h>
#include <chrono>

namespace Flare
{
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

            ~RecordsBuffer()
            {
                delete[] Records;
            }

            inline bool IsEmpty() const { return Size == 0; }
            inline bool IsFull() const { return Size == Capacity; }

            inline void AddRecord(const char* name, uint64_t start, uint64_t end)
            {
                FLARE_CORE_ASSERT(Size < Capacity);

                Records[Size] = Record{ name, start, end };
                Size++;
            }

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

        static size_t GetRecordsCount();

        static bool IsRecording();
        static size_t GetBuffersCount();
        static const RecordsBuffer& GetRecordsBuffer(size_t index);
        static const std::vector<Frame>& GetFrames();

        static void ClearData();

        static void SubmitRecord(const char* name, uint64_t start, uint64_t end);
    };

    struct ProfilerScopeTimer
    {
    public:
        ProfilerScopeTimer(const char* name)
            : m_Name(name)
        {
            m_StartTime = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
        }

        ~ProfilerScopeTimer()
        {
            uint64_t end = (uint64_t) std::chrono::high_resolution_clock::now().time_since_epoch().count();
            Profiler::SubmitRecord(m_Name, m_StartTime, end);
        }
    private:
        const char* m_Name;
        uint64_t m_StartTime;
    };

#define FLARE_PROFILE_TIMER_NAME2(line) ___profileTimer___##line
#define FLARE_PROFILE_TIMER_NAME(line) FLARE_PROFILE_TIMER_NAME2(line)
#define FLARE_PROFILE_SCOPE(name) Flare::ProfilerScopeTimer FLARE_PROFILE_TIMER_NAME(__LINE__) = Flare::ProfilerScopeTimer(name)

#define FLARE_PROFILE_FUNCTION() FLARE_PROFILE_SCOPE(__FUNCSIG__)
}