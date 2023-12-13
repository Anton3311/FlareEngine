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
        m_Zoom(1.0f),
        m_ScrollOffset(0.0f),
        m_ScrollSpeed(10.0f)
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

            const auto& lastRecordsBuffer = Profiler::GetRecordsBuffer(buffersCount - 1);

            uint64_t profileStartTime = Profiler::GetRecordsBuffer(0).Records[0].StartTime;
            uint64_t profileEndTime = lastRecordsBuffer.Records[lastRecordsBuffer.Size - 1].EndTime;

            float maxScrollOffset = CalculatePositionFromTime(profileEndTime - profileStartTime) + m_ScrollOffset;
            float scroll = ImGui::GetIO().MouseWheel;
            if (glm::abs(scroll) > 0.0f)
            {
                if (ImGui::IsKeyDown(ImGuiKey_ModShift))
                {
                    m_ScrollOffset -= scroll * m_ScrollSpeed;
                }
                else
                {
                    const float maxZoomSpeed = 2.0f;
                    const float minZoomSpeed = 0.001f;

                    float zoomAmount = glm::sign(scroll) * glm::clamp(glm::exp(0.2f * m_Zoom - 4.0f), minZoomSpeed, maxZoomSpeed);
                    float mouseCursorOffset = ImGui::GetIO().MousePos.x - ImGui::GetCurrentWindow()->Pos.x - initialCursorPosition.x;
                    float initialPosition = mouseCursorOffset;

                    uint64_t timePointUnderCursor = CalculateTimeFromPosition(mouseCursorOffset);

                    m_Zoom = glm::clamp(m_Zoom + zoomAmount, 0.001f, 1000.0f);

                    float position = CalculatePositionFromTime(timePointUnderCursor);

                    m_ScrollOffset += position - initialPosition;
                }
            }

            m_ScrollOffset = glm::clamp(m_ScrollOffset, 0.0f, maxScrollOffset);

            for (size_t bufferIndex = 0; bufferIndex < buffersCount; bufferIndex++)
            {
                const Profiler::RecordsBuffer& buffer = Profiler::GetRecordsBuffer(bufferIndex);
                for (size_t i = 0; i < buffer.Size; i++)
                {
                    const Profiler::Record& record = buffer.Records[i];
                    size_t recordIndex = i + bufferIndex * Profiler::GetRecordsCountPerBuffer();

                    size_t row = CalculateRecordRow(recordIndex, record);
                    ImVec2 offset = ImVec2(CalculatePositionFromTime(record.StartTime - profileStartTime), (m_BlockHeight + 2.0f) * ((float)row));

                    Profiler::Record recordCopy = record;
                    recordCopy.StartTime -= profileStartTime;
                    recordCopy.EndTime -= profileStartTime;

                    if (printBlockInfo)
                    {
                        FLARE_CORE_INFO("{} Row: {} Position: {} {}ms -> {}ms", record.Name, row, offset.x,
                            (double)(record.StartTime - profileStartTime) / 1000000.0,
                            (double)(record.EndTime - profileStartTime) / 1000000.0);
                    }

                    DrawRecordBlock(recordCopy, offset + initialCursorPosition, drawList);
                }
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

        ImGui::DragFloat("Scale", &m_Zoom);
        ImGui::DragFloat("Offset", &m_ScrollOffset, 1.0f, -10000000.0f, 10000000.0f);
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
        double position = (start) * (double)(m_WindowWidth * m_Zoom);
        return (float)position - m_ScrollOffset;
    }

    uint64_t ProfilerWindow::CalculateTimeFromPosition(float position)
    {
        position += m_ScrollOffset;
        double milliseconds = (double)(position) / (double)(m_WindowWidth * m_Zoom);
        return (uint64_t)(milliseconds * 1000000.0);
    }
}
