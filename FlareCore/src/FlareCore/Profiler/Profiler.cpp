#include "Profiler.h"

#include "FlareCore/Assert.h"

namespace Flare
{
    struct ProfilerData
    {
        std::vector<Profiler::RecordsBuffer> Buffers;
        std::vector<Profiler::Frame> Frames;

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
        }

        if (!s_ProfilerData.IsRecording)
            return;

        Profiler::Frame& frame = s_ProfilerData.Frames.emplace_back();

        if (s_ProfilerData.Buffers.empty())
        {
            frame.FirstRecordIndex = 0;
        }
    }

    void Profiler::EndFrame()
    {
        if (s_ProfilerData.RecordingStopRequested)
        {
            s_ProfilerData.IsRecording = false;
            s_ProfilerData.RecordingStopRequested = false;
        }

        if (!s_ProfilerData.IsRecording)
            return;

        FLARE_CORE_ASSERT(!s_ProfilerData.Frames.empty());

        Frame& currentFrame = s_ProfilerData.Frames.back();

        if (s_ProfilerData.Buffers.empty())
        {
            currentFrame.RecordsCount = 0;
        }
        else
        {
            currentFrame.RecordsCount = s_ProfilerData.Buffers.back().Size;
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
    }

    inline static Profiler::RecordsBuffer& GetCurrentBuffer()
    {
        if (s_ProfilerData.Buffers.empty())
            return s_ProfilerData.Buffers.emplace_back(s_ProfilerData.BufferSize);

        if (s_ProfilerData.Buffers.back().IsFull())
            return s_ProfilerData.Buffers.emplace_back(s_ProfilerData.BufferSize);

        return s_ProfilerData.Buffers.back();
    }

    Profiler::Record* Profiler::CreateRecord()
    {
        if (!s_ProfilerData.IsRecording)
            return nullptr;

        auto& buffer = GetCurrentBuffer();
        buffer.Size++;
        return &buffer.Records[buffer.Size - 1];
    }
}
