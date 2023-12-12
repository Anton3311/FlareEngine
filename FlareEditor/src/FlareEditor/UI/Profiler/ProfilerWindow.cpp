#include "ProfilerWindow.h"

#include "FlareEditor/ImGui/ImGuiLayer.h"
#include "FlareCore/Profiler/Profiler.h"

#include <imgui_internal.h>
#include <glm/glm.hpp>

namespace Flare
{
    ProfilerWindow* ProfilerWindow::s_Instance = nullptr;

    ProfilerWindow::ProfilerWindow()
        : m_ShowWindow(false),
        m_BlockHeight(30.0f),
        m_WindowWidth(0.0f),
        m_Scale(10.0f),
        m_ScrollOffset(0.0f)
    {
        s_Instance = this;
    }

    ProfilerWindow::~ProfilerWindow()
    {
        s_Instance = nullptr;
    }

    static bool printBlockInfo = false;

    void ProfilerWindow::OnImGuiRender()
    {
        if (!m_ShowWindow)
            return;

        ImGui::Begin("Profiler", &m_ShowWindow);

        RenderToolBar();

        ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;

        ImGui::BeginChild("ProfilerDataPreviewArea");

        ImVec2 initialCursorPosition = ImGui::GetCursorPos();
        ImVec2 contentAreaSize = ImGui::GetContentRegionAvail();
        m_WindowWidth = contentAreaSize.x;

        bool canMoveViewport = ImGui::IsItemHovered() && ImGui::IsWindowFocused();
        bool hasProfileData = !Profiler::IsRecording() && Profiler::GetFrames().size() > 0;

        if (!hasProfileData)
        {
            ImGui::InvisibleButton("ProfilerViewport", contentAreaSize);
            ImRect rect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
            const char* text = "No profile data";
            ImVec2 textSize = ImGui::CalcTextSize(text);
            drawList->AddText(rect.GetCenter() - textSize / 2.0f, 0xffffffff, text);
        }
        else
        {
            m_RecordsStack.clear();

            size_t recordsPerBuffer = Profiler::GetRecordsCountPerBuffer();
            size_t buffersCount = Profiler::GetBuffersCount();
            const auto& frames = Profiler::GetFrames();

            uint64_t profileStartTime = Profiler::GetRecordsBuffer(0).Records[0].StartTime;
            const auto& frame = Profiler::GetFrames()[0];

            for (size_t bufferIndex = 0; bufferIndex < buffersCount; bufferIndex++)
            {
                bool stop = false;
                const Profiler::RecordsBuffer& buffer = Profiler::GetRecordsBuffer(bufferIndex);

                for (size_t i = 0; i < buffer.Size; i++)
                {
                    const Profiler::Record& record = buffer.Records[i];
                    size_t recordIndex = i + bufferIndex * Profiler::GetRecordsCountPerBuffer();

                    if (recordIndex >= frame.RecordsCount)
                    {
                        stop = true;
                        break;
                    }

                    size_t row = CalculateRecordRow(recordIndex, record);
                    ImVec2 offset = ImVec2(CalculatePositionFromTime(record.StartTime - profileStartTime), (m_BlockHeight + 2.0f) * ((float)row));

                    if (printBlockInfo)
                    {
                        FLARE_CORE_INFO("Row: {} Position: {} {} -> {}", row, offset.x,
                            (double)(record.StartTime - profileStartTime) / 1000000.0,
                            (double)(record.EndTime - profileStartTime) / 1000000.0);
                    }

                    DrawRecordBlock(record, offset + initialCursorPosition, drawList);
                }

                if (stop)
                    break;
            }

            printBlockInfo = false;
        }

        ImGui::EndChild();

        ImGui::End();
    }

    void ProfilerWindow::RenderToolBar()
    {
        ImGui::BeginDisabled(Profiler::IsRecording());
        if (ImGui::Button("Start recording"))
        {
            printBlockInfo = true;
            Profiler::ClearData();
            Profiler::StartRecording();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!Profiler::IsRecording());
        if (ImGui::Button("Stop recording"))
        {
            Profiler::StopRecording();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::Text("Frames recorded: %d", (uint32_t)Profiler::GetFrames().size());

        ImGui::DragFloat("Scale", &m_Scale);
        ImGui::DragFloat("Scroll offset", &m_ScrollOffset);
    }

    void ProfilerWindow::DrawRecordBlock(const Profiler::Record& record, ImVec2 position, ImDrawList* drawList)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        double duration = (double)(record.EndTime - record.StartTime);

        double milliseconds = duration / 1000000.0;
        double seconds = milliseconds / 1000.0;

        float width = glm::abs(CalculatePositionFromTime(record.StartTime) - CalculatePositionFromTime(record.EndTime));

        if (printBlockInfo)
            FLARE_CORE_INFO(width);

        width = glm::max(width, 1.0f);

        ImGui::SetCursorPos(position);

        ImVec2 blockSize = ImVec2(width, m_BlockHeight);
        ImGui::InvisibleButton(record.Name, blockSize);

        ImRect blockRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
        
        drawList->AddRectFilled(blockRect.Min, blockRect.Max, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_FrameBg]));

        const char* blockName = record.Name;
        if (width < 10.0f)
            blockName = "";

        const char* text;
        const char* textEnd;

        if (milliseconds < 0.001f)
            ImFormatStringToTempBuffer(&text, &textEnd, "%s %f ns", blockName, (float)duration);
        else if (seconds < 0.01f)
            ImFormatStringToTempBuffer(&text, &textEnd, "%s %f ms", blockName, (float)milliseconds);
        else
            ImFormatStringToTempBuffer(&text, &textEnd, "%s %f s", blockName, (float)seconds);


        ImVec2 textSize = ImGui::CalcTextSize(text, textEnd);

        drawList->PushClipRect(blockRect.Min - ImVec2(10.0f, 0.0f), blockRect.Max + ImVec2(10.0f, 0.0f));
        drawList->AddText(blockRect.Min + blockSize / 2.0f - textSize / 2.0f, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]), text, textEnd);
        drawList->PopClipRect();
    }

    size_t ProfilerWindow::CalculateRecordRow(size_t currentRecordIndex, const Profiler::Record& currentRecord)
    {
        size_t eraseFromIndex = 0;
        for (size_t recordIndex : m_RecordsStack)
        {
            size_t bufferIndex = recordIndex / Profiler::GetRecordsCountPerBuffer();
            size_t recordIndexInBuffer = recordIndex % Profiler::GetRecordsCountPerBuffer();

            const auto& buffer = Profiler::GetRecordsBuffer(bufferIndex);
            const auto& record = buffer.Records[recordIndexInBuffer];

            if (currentRecord.StartTime >= record.EndTime)
            {
                m_RecordsStack.erase(m_RecordsStack.begin() + eraseFromIndex, m_RecordsStack.end());
                break;
            }
            else
            {
                eraseFromIndex++;
            }
        }

        m_RecordsStack.push_back(currentRecordIndex);
        return eraseFromIndex;
    }

    float ProfilerWindow::CalculatePositionFromTime(uint64_t time)
    {
        double start = (double)time / 1000000.0;
        double position = (start) * (double)(m_WindowWidth * m_Scale);
        return (float)position - m_ScrollOffset;
    }
}
