#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"

#include <optional>

namespace Flare
{
    class ProfilerWindow
    {
    public:
        ProfilerWindow();
        ~ProfilerWindow();

        void OnImGuiRender();

        inline static ProfilerWindow& GetInstance()
        {
            FLARE_CORE_ASSERT(s_Instance);
            return *s_Instance;
        }

        inline void ShowWindow() { m_ShowWindow = true; }
    private:
        void RenderToolBar();
        void DrawRecordBlock(const Profiler::Record& record, ImVec2 position, ImDrawList* drawList, size_t recordIndex);
        void RenderSideBar();

        size_t CalculateRecordRow(size_t currentRecordIndex, const Profiler::Record& currentRecord);
        float CalculatePositionFromTime(uint64_t time);
        uint64_t CalculateTimeFromPosition(float position);

        void RenderSubCallsList();
        void RenderCallStack();
        void RenderRecordTableRow(size_t recordIndex);

        void ReconstructSubCallsList(size_t start);
        void ReconstructCallStack(size_t start);
        void CollectAverageTimeData();
    private:
        static ProfilerWindow* s_Instance;

        struct Record
        {
            const char* Name;
            float AverageTimeInMilliseconds;
        };

        std::vector<size_t> m_RecordsStack;
        std::vector<Record> m_RecordsByAverageTime;

        std::optional<size_t> m_PreviousSelection;
        std::optional<size_t> m_SelectedRecord;

        size_t m_FirstSubCallRecordIndex;
        size_t m_SubCallRecordsCount;
        std::vector<size_t> m_CallStackRecords;

        ImVec2 m_PreviousMousePosition;

        float m_ScrollSpeed;
        float m_BlockHeight;
        float m_ScrollOffset;
        float m_Zoom;

        float m_WindowWidth;
        bool m_ShowWindow;
    };
}