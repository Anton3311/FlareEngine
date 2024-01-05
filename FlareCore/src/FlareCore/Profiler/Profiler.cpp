#include "Profiler.h"

#include "FlareCore/Assert.h"

#include <Windows.h>

namespace Flare
{
    struct ProfilerData
    {
        std::vector<Profiler::RecordsBuffer> Buffers;
        std::vector<Profiler::Frame> Frames;

        size_t CurrentBufferIndex = 0;
        size_t RecordsRecorded = 0;

        bool IsRecording = false;
        bool RecordingStartRequested = false;
        bool RecordingStopRequested = false;

        size_t BufferSize = 40 * 1000;
    };

    ProfilerData s_ProfilerData;

    void Profiler::BeginFrame()
    {
        if (s_ProfilerData.RecordingStartRequested)
        {
            s_ProfilerData.IsRecording = true;
            s_ProfilerData.RecordingStartRequested = false;
            s_ProfilerData.CurrentBufferIndex = 0;
            s_ProfilerData.RecordsRecorded = 0;
        }

        if (!s_ProfilerData.IsRecording)
            return;

        Profiler::Frame& frame = s_ProfilerData.Frames.emplace_back();
        frame.FirstRecordIndex = s_ProfilerData.RecordsRecorded;
    }

    void Profiler::EndFrame()
    {
        if (!s_ProfilerData.IsRecording)
            return;

        FLARE_CORE_ASSERT(!s_ProfilerData.Frames.empty());

        Frame& currentFrame = s_ProfilerData.Frames.back();
        currentFrame.RecordsCount = s_ProfilerData.RecordsRecorded - currentFrame.FirstRecordIndex;

        if (s_ProfilerData.RecordingStopRequested)
        {
            s_ProfilerData.IsRecording = false;
            s_ProfilerData.RecordingStopRequested = false;
        }
    }

    void Profiler::StartRecording()
    {
        if (!s_ProfilerData.IsRecording)
            s_ProfilerData.RecordingStartRequested = true;
    }

    void Profiler::StopRecording()
    {
        if (s_ProfilerData.IsRecording)
            s_ProfilerData.RecordingStopRequested = true;
    }

    size_t Profiler::GetRecordsCountPerBuffer()
    {
        return s_ProfilerData.BufferSize;
    }

    size_t Profiler::GetRecordsCount()
    {
        if (s_ProfilerData.Buffers.empty())
            return 0;

        return s_ProfilerData.BufferSize * (s_ProfilerData.Buffers.size() - 1) + s_ProfilerData.Buffers.back().Size;
    }

    Profiler::Record& Profiler::GetRecord(size_t index)
    {
        size_t bufferIndex = index / s_ProfilerData.BufferSize;
        size_t recordIndex = index % s_ProfilerData.BufferSize;

        FLARE_CORE_ASSERT(bufferIndex <= s_ProfilerData.CurrentBufferIndex);
        FLARE_CORE_ASSERT(recordIndex < s_ProfilerData.Buffers[bufferIndex].Size);

        return s_ProfilerData.Buffers[bufferIndex].Records[recordIndex];
    }

    bool Profiler::IsRecording()
    {
        return s_ProfilerData.IsRecording;
    }

    size_t Profiler::GetBuffersCount()
    {
        return s_ProfilerData.Buffers.size();
    }

    const Profiler::RecordsBuffer& Profiler::GetRecordsBuffer(size_t index)
    {
        FLARE_CORE_ASSERT(index < s_ProfilerData.Buffers.size());
        return s_ProfilerData.Buffers[index];
    }

    const std::vector<Profiler::Frame>& Profiler::GetFrames()
    {
        return s_ProfilerData.Frames;
    }

    void Profiler::ClearData()
    {
        s_ProfilerData.Buffers.clear();
        s_ProfilerData.Frames.clear();
        s_ProfilerData.CurrentBufferIndex = 0;
        s_ProfilerData.RecordsRecorded = 0;
    }

    inline static Profiler::RecordsBuffer& GetCurrentBuffer()
    {
        if (s_ProfilerData.CurrentBufferIndex < s_ProfilerData.Buffers.size())
        {
            Profiler::RecordsBuffer& currentBuffer = s_ProfilerData.Buffers[s_ProfilerData.CurrentBufferIndex];
            if (currentBuffer.Size < currentBuffer.Capacity)
                return s_ProfilerData.Buffers[s_ProfilerData.CurrentBufferIndex];
            
            if (s_ProfilerData.CurrentBufferIndex + 1 < s_ProfilerData.Buffers.size())
            {
                s_ProfilerData.CurrentBufferIndex++;
                return s_ProfilerData.Buffers[s_ProfilerData.CurrentBufferIndex];
            }
            else
            {
                s_ProfilerData.CurrentBufferIndex++;
                return s_ProfilerData.Buffers.emplace_back(s_ProfilerData.BufferSize);
            }
        }

        s_ProfilerData.CurrentBufferIndex = s_ProfilerData.Buffers.size();
        return s_ProfilerData.Buffers.emplace_back(s_ProfilerData.BufferSize);
    }

    Profiler::Record* Profiler::CreateRecord()
    {
        if (!s_ProfilerData.IsRecording)
            return nullptr;

        s_ProfilerData.RecordsRecorded++;

        auto& buffer = GetCurrentBuffer();
        buffer.Size++;
        return &buffer.Records[buffer.Size - 1];
    }

    ProfilerScopeTimer::ProfilerScopeTimer(const char* name)
    {
        m_Record = Profiler::CreateRecord();

        if (m_Record)
        {
            m_Record->Name = name;
            m_Record->StartTime = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
        }
    }

    ProfilerScopeTimer::~ProfilerScopeTimer()
    {
        if (m_Record)
        {
            m_Record->EndTime = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
        }
    }
}
