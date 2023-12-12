#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"

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
        void DrawRecordBlock(const Profiler::Record& record, ImVec2 position, ImDrawList* drawList);
        size_t CalculateRecordRow(size_t currentRecordIndex, const Profiler::Record& currentRecord);
        float CalculatePositionFromTime(uint64_t time);
    private:
        static ProfilerWindow* s_Instance;

        std::vector<size_t> m_RecordsStack;

        float m_BlockHeight;
        float m_ScrollOffset;
        float m_Scale;

        float m_WindowWidth;
        bool m_ShowWindow;
    };
}