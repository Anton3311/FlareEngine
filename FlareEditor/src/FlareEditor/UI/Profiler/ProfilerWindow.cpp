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

    static double s_MillisecondToNanosecond = 1000000.0;

    void ProfilerWindow::OnImGuiRender()
    {
        if (!m_ShowWindow)
            return;

        ImGui::Begin("Profiler", &m_ShowWindow, ImGuiWindowFlags_NoScrollbar);

        RenderToolBar();

        ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;
        float sideBarWidth = 600.0f;

        ImVec2 initialCursorPosition = ImGui::GetCursorPos();
        ImVec2 contentAreaSize = ImGui::GetContentRegionAvail();

        contentAreaSize.x -= sideBarWidth;

        ImGui::BeginChild("ProfilerDataPreviewArea", contentAreaSize);
        m_WindowWidth = contentAreaSize.x;

        ImGui::InvisibleButton("ProfilerViewport", contentAreaSize);
        ImGui::SetItemUsingMouseWheel();
        ImRect viewportRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

        bool canInteractWithViewport = ImGui::IsItemHovered();
        bool hasProfileData = !Profiler::IsRecording() && Profiler::GetFrames().size() > 0;

        bool isDragging = ImGui::IsMouseDragging(ImGuiMouseButton_Middle);

        float dragInput = 0.0f;

        ImVec2 currentMousePosition = ImGui::GetIO().MousePos;
        if (ImGui::IsItemClicked())
        {
            m_PreviousMousePosition = currentMousePosition;
        }

        if (canInteractWithViewport && isDragging)
        {
            dragInput = currentMousePosition.x - m_PreviousMousePosition.x;
        }

        m_PreviousMousePosition = currentMousePosition;

        if (!hasProfileData)
        {
            const char* text = "No profile data";
            ImVec2 textSize = ImGui::CalcTextSize(text);
            drawList->AddText(viewportRect.GetCenter() - textSize / 2.0f, 0xffffffff, text);
        }
        else
        {
            size_t buffersCount = Profiler::GetBuffersCount();
            const auto& lastRecordsBuffer = Profiler::GetRecordsBuffer(buffersCount - 1);

            uint64_t profileStartTime = Profiler::GetRecordsBuffer(0).Records[0].StartTime;
            uint64_t profileEndTime = lastRecordsBuffer.Records[lastRecordsBuffer.Size - 1].EndTime;

            float maxScrollOffset = CalculatePositionFromTime(profileEndTime - profileStartTime) + m_ScrollOffset;
            float scroll = ImGui::GetIO().MouseWheel;

            // Handle zooming and scrolling

            if (isDragging && glm::abs(dragInput) > 0.0f)
            {
                m_ScrollOffset -= dragInput;
            }
            else if (canInteractWithViewport && glm::abs(scroll) > 0.0f)
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

            RenderTimeLine(viewportRect, initialCursorPosition);

            if (m_SelectedRecord != m_PreviousSelection && m_SelectedRecord.has_value())
            {
                ReconstructSubCallsList(m_SelectedRecord.value());
                ReconstructCallStack(m_SelectedRecord.value());
            }

            m_PreviousSelection = m_SelectedRecord;
        }

        ImGui::EndChild();

        const auto& style = ImGui::GetStyle();
        drawList->AddLine(ImVec2(viewportRect.Max.x, viewportRect.Min.y), viewportRect.Max, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Border]));

        ImGui::SetCursorPos(ImVec2(initialCursorPosition.x + contentAreaSize.x + style.ItemSpacing.x, initialCursorPosition.y));
        RenderSideBar();

        // Records by average time
        ImGui::End();

        if (ImGui::Begin("Average Execution Time"))
            RenderAverageTimeData();
        ImGui::End();
    }

    void ProfilerWindow::RenderToolBar()
    {
        ImGui::BeginDisabled(Profiler::IsRecording());
        if (ImGui::Button("Start recording"))
        {
            Profiler::ClearData();
            Profiler::StartRecording();

            m_PreviousSelection = {};
            m_SelectedRecord = {};
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(!Profiler::IsRecording());
        if (ImGui::Button("Stop recording"))
        {
            Profiler::StopRecording();

            CollectAverageTimeData();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::Text("Frames recorded: %d", (uint32_t)Profiler::GetFrames().size());
    }

    void ProfilerWindow::DrawRecordBlock(const Profiler::Record& record, ImVec2 position, ImDrawList* drawList, size_t recordIndex)
    {
        ImGuiStyle& style = ImGui::GetStyle();

        double duration = (double)(record.EndTime - record.StartTime);

        double milliseconds = duration / s_MillisecondToNanosecond;
        double seconds = milliseconds / 1000.0;

        float width = glm::abs(CalculatePositionFromTime(record.StartTime) - CalculatePositionFromTime(record.EndTime));
        if (width < 1.0f)
            return;

        ImGui::SetCursorPos(position);

        ImVec2 blockSize = ImVec2(width, m_BlockHeight);
        ImGui::InvisibleButton(record.Name, blockSize);

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_SelectedRecord = recordIndex;
        }

        ImRect blockRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
        
        ImU32 blockColor = ImGui::ColorConvertFloat4ToU32(
            recordIndex == m_SelectedRecord
            ? style.Colors[ImGuiCol_FrameBgHovered]
            : style.Colors[ImGuiCol_FrameBg]);

        drawList->AddRectFilled(blockRect.Min, blockRect.Max, blockColor);

        const char* blockName = record.Name;
        
        const char* text;
        const char* textEnd;

        if (milliseconds < 0.001f)
            ImFormatStringToTempBuffer(&text, &textEnd, "%s %f ns", blockName, (float)duration);
        else if (seconds < 0.01f)
            ImFormatStringToTempBuffer(&text, &textEnd, "%s %f ms", blockName, (float)milliseconds);
        else
            ImFormatStringToTempBuffer(&text, &textEnd, "%s %f s", blockName, (float)seconds);

        ImVec2 textSize = ImGui::CalcTextSize(text, textEnd);

        drawList->PushClipRect(blockRect.Min, blockRect.Max, true);
        drawList->AddText(blockRect.Min + blockSize / 2.0f - textSize / 2.0f, ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]), text, textEnd);
        drawList->PopClipRect();
    }

    void ProfilerWindow::RenderSideBar()
    {
        ImVec2 contentAreaSize = ImGui::GetContentRegionAvail();

        ImGui::BeginChild("ProfilerSideBar", contentAreaSize);
        
        if (m_SelectedRecord.has_value())
        {
            size_t bufferIndex = m_SelectedRecord.value() / Profiler::GetRecordsCountPerBuffer();
            size_t recordIndex = m_SelectedRecord.value() % Profiler::GetRecordsCountPerBuffer();
            const Profiler::Record& record = Profiler::GetRecordsBuffer(bufferIndex).Records[recordIndex];

            ImGui::TextWrapped(record.Name);

            ImGui::SeparatorText("Sub Calls");

            ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;
            if (ImGui::TreeNodeEx("Sub Calls", treeNodeFlags))
            {
                RenderSubCallsList();
                ImGui::TreePop();
            }

            if (ImGui::TreeNodeEx("Call Stack", treeNodeFlags))
            {
                RenderCallStack();
                ImGui::TreePop();
            }

        }
        else
        {
            ImGui::Text("Nothing selected");
        }

        ImGui::EndChild();
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
        double start = (double)time / s_MillisecondToNanosecond;
        double position = (start) * (double)(m_WindowWidth * m_Zoom);
        return (float)position - m_ScrollOffset;
    }

    uint64_t ProfilerWindow::CalculateTimeFromPosition(float position)
    {
        position += m_ScrollOffset;
        double milliseconds = (double)(position) / (double)(m_WindowWidth * m_Zoom);
        return (uint64_t)(milliseconds * s_MillisecondToNanosecond);
    }

    void ProfilerWindow::RenderSubCallsList()
    {
        ImVec2 contentAreaSize = ImGui::GetContentRegionAvail();

        ImGui::BeginTable("SubCallsTable", 2);

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, contentAreaSize.x * 0.80f);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, contentAreaSize.x * 0.20f);

        for (size_t i = m_FirstSubCallRecordIndex; i < m_FirstSubCallRecordIndex + m_SubCallRecordsCount; i++)
        {
            RenderRecordTableRow(i);
        }

        ImGui::EndTable();
    }

    void ProfilerWindow::RenderCallStack()
    {
        ImVec2 contentAreaSize = ImGui::GetContentRegionAvail();

        ImGui::BeginTable("CallStackTable", 2);

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, contentAreaSize.x * 0.80f);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, contentAreaSize.x * 0.20f);

        for (size_t i : m_CallStackRecords)
        {
            RenderRecordTableRow(i);
        }

        ImGui::EndTable();
    }

    void ProfilerWindow::RenderRecordTableRow(size_t index)
    {
        const auto& style = ImGui::GetStyle();

        size_t bufferIndex = index / Profiler::GetRecordsCountPerBuffer();
        size_t recordIndex = index % Profiler::GetRecordsCountPerBuffer();

        const auto& record = Profiler::GetRecordsBuffer(bufferIndex).Records[recordIndex];

        ImGui::TableNextRow(0, ImGui::GetFontSize() + style.FramePadding.y * 2.0f);
        ImGui::TableSetColumnIndex(0);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
        ImGui::TextUnformatted(record.Name);

        ImGui::TableSetColumnIndex(1);

        double duration = (double)(record.EndTime - record.StartTime);

        double milliseconds = duration / s_MillisecondToNanosecond;
        double seconds = milliseconds / 1000.0;

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);

        if (milliseconds < 0.001f)
            ImGui::Text("%f ns", duration);
        else if (seconds < 0.01f)
            ImGui::Text("%f ms", milliseconds);
        else
            ImGui::Text("%f s", seconds);
    }

    void ProfilerWindow::RenderAverageTimeData()
    {
        const auto& style = ImGui::GetStyle();
        ImVec2 windowSize = ImGui::GetContentRegionAvail();

        ImGuiListClipper clipper;
        clipper.Begin((int32_t)m_RecordsByAverageTime.size());

        while (clipper.Step())
        {
            if (ImGui::BeginTable("AverageTime", 2))
            {
                ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, windowSize.x * 0.10f);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, windowSize.x * 0.90f);

                for (int32_t i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
                {
                    if (i < 0 || i >= (int32_t)m_RecordsByAverageTime.size())
                        continue;

                    ImGui::TableNextRow();

                    ImGui::TableNextRow(0, ImGui::GetFontSize() + style.FramePadding.y * 2.0f);
                    ImGui::TableSetColumnIndex(0);

                    ImGui::Text("%f ms", m_RecordsByAverageTime[i].AverageTimeInMilliseconds);

                    ImGui::TableSetColumnIndex(1);

                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + style.FramePadding.y);
                    ImGui::TextUnformatted(m_RecordsByAverageTime[i].Name);
                }

                ImGui::EndTable();
            }
        }
    }

    void ProfilerWindow::RenderTimeLine(ImRect viewportRect, ImVec2 position)
    {
        ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;

        size_t recordsPerBuffer = Profiler::GetRecordsCountPerBuffer();
        size_t buffersCount = Profiler::GetBuffersCount();
        const auto& frames = Profiler::GetFrames();

        const auto& lastRecordsBuffer = Profiler::GetRecordsBuffer(buffersCount - 1);

        uint64_t profileStartTime = Profiler::GetRecordsBuffer(0).Records[0].StartTime;
        uint64_t profileEndTime = lastRecordsBuffer.Records[lastRecordsBuffer.Size - 1].EndTime;

        m_RecordsStack.clear();
        drawList->PushClipRect(viewportRect.Min, viewportRect.Max);

        size_t firstVisibleFrameIndex = 0;
        size_t lastVisibleFrameIndex = 0;
        for (size_t i = 0; i < frames.size(); i++)
        {
            const Profiler::Frame& frame = frames[i];
            if (frame.RecordsCount == 0)
                continue;

            uint64_t frameStartTime = Profiler::GetRecord(frame.FirstRecordIndex).StartTime;
            uint64_t frameEndTime = Profiler::GetRecord(frame.FirstRecordIndex + frame.RecordsCount - 1).EndTime;

            float startPosition = CalculatePositionFromTime(frameStartTime - profileStartTime);
            float endPosition = CalculatePositionFromTime(frameEndTime - profileStartTime);

            if (endPosition <= 0.0f)
            {
                firstVisibleFrameIndex++;
                continue;
            }

            if (startPosition >= viewportRect.Max.x - viewportRect.Min.x)
            {
                // All the other frames have records that will be rendered
                // outside the timeline bounds, so there is no point in processing them
                lastVisibleFrameIndex = i;
                break;
            }

            if (i == frames.size() - 1)
                lastVisibleFrameIndex = i;
        }

        size_t firstBuffer = frames[firstVisibleFrameIndex].FirstRecordIndex / Profiler::GetRecordsCountPerBuffer();
        size_t lastBuffer = (frames[lastVisibleFrameIndex].FirstRecordIndex + frames[lastVisibleFrameIndex].RecordsCount - 1) / Profiler::GetRecordsCountPerBuffer();

        size_t firstVisibleRecord = frames[firstVisibleFrameIndex].FirstRecordIndex % Profiler::GetRecordsCountPerBuffer();
        size_t lastVisibleRecord = (frames[lastVisibleFrameIndex].FirstRecordIndex + frames[lastVisibleFrameIndex].RecordsCount - 1) % Profiler::GetRecordsCountPerBuffer();

        for (size_t bufferIndex = firstBuffer; bufferIndex <= lastBuffer; bufferIndex++)
        {
            const Profiler::RecordsBuffer& buffer = Profiler::GetRecordsBuffer(bufferIndex);

            size_t firstRecord = 0;
            size_t lastRecord = buffer.Size - 1;

            if (bufferIndex == firstBuffer)
                firstRecord = firstVisibleRecord;
            if (bufferIndex == lastBuffer)
                lastRecord = lastVisibleRecord;

            for (size_t i = firstRecord; i <= lastRecord; i++)
            {
                const Profiler::Record& record = buffer.Records[i];
                size_t recordIndex = i + bufferIndex * Profiler::GetRecordsCountPerBuffer();

                size_t row = CalculateRecordRow(recordIndex, record);
                ImVec2 offset = ImVec2(CalculatePositionFromTime(record.StartTime - profileStartTime), (m_BlockHeight + 2.0f) * ((float)row));

                Profiler::Record recordCopy = record;
                recordCopy.StartTime -= profileStartTime;
                recordCopy.EndTime -= profileStartTime;

                DrawRecordBlock(recordCopy, offset + position, drawList, recordIndex);
            }
        }

        drawList->PopClipRect();
    }

    void ProfilerWindow::ReconstructSubCallsList(size_t start)
    {
        m_FirstSubCallRecordIndex = start + 1;
        m_SubCallRecordsCount = 0;

        size_t currentBufferIndex = start / Profiler::GetRecordsCountPerBuffer();
        size_t recordIndex = start % Profiler::GetRecordsCountPerBuffer();

        uint64_t endTime = Profiler::GetRecordsBuffer(currentBufferIndex).Records[recordIndex].EndTime;

        while (currentBufferIndex < Profiler::GetBuffersCount())
        {
            const auto& currentRecord = Profiler::GetRecordsBuffer(currentBufferIndex).Records[recordIndex];
            if (currentRecord.EndTime > endTime)
            {
                break;
            }

            recordIndex++;

            m_SubCallRecordsCount++;

            if (recordIndex > Profiler::GetRecordsCountPerBuffer())
            {
                currentBufferIndex++;
                recordIndex = 0;
            }
        }
    }

    void ProfilerWindow::ReconstructCallStack(size_t start)
    {
        m_CallStackRecords.clear();
        
        size_t currentBufferIndex = start / Profiler::GetRecordsCountPerBuffer();
        size_t recordIndex = start % Profiler::GetRecordsCountPerBuffer();

        const Profiler::Record& startRecord = Profiler::GetRecordsBuffer(currentBufferIndex).Records[recordIndex];

        while (true)
        {
            const auto& currentRecord = Profiler::GetRecordsBuffer(currentBufferIndex).Records[recordIndex];
            if (currentRecord.StartTime <= startRecord.StartTime && currentRecord.EndTime >= startRecord.EndTime)
            {
                m_CallStackRecords.push_back(currentBufferIndex * Profiler::GetRecordsCountPerBuffer() + recordIndex);
            }

            if (recordIndex == 0)
            {
                if (currentBufferIndex == 0)
                    break;

                currentBufferIndex--;
                recordIndex = Profiler::GetRecordsCountPerBuffer() - 1;
            }
            else
            {
                recordIndex--;
            }
        }

        for (size_t i = 0; i < m_CallStackRecords.size() / 2; i++)
        {
            std::swap(m_CallStackRecords[i], m_CallStackRecords[m_CallStackRecords.size() - 1 - i]);
        }
    }

    void ProfilerWindow::CollectAverageTimeData()
    {
        struct RecordData
        {
            float Time;
            uint32_t RecordsCount;
        };

        std::unordered_map<const char*, RecordData> records;

        for (size_t bufferIndex = 0; bufferIndex < Profiler::GetBuffersCount(); bufferIndex++)
        {
            const Profiler::RecordsBuffer& buffer = Profiler::GetRecordsBuffer(bufferIndex);
            for (size_t i = 0; i < buffer.Size; i++)
            {
                const Profiler::Record& record = buffer.Records[i];

                double duration = (double)(record.EndTime - record.StartTime);
                double milliseconds = duration / s_MillisecondToNanosecond;

                auto it = records.find(record.Name);
                if (it == records.end())
                    records.emplace(record.Name, RecordData{ (float)milliseconds, 1 });
                else
                {
                    it->second.RecordsCount++;
                    it->second.Time += (float)milliseconds;
                }
            }
        }

        m_RecordsByAverageTime.clear();
        m_RecordsByAverageTime.reserve(records.size());

        for (const auto& [name, data] : records)
        {
            Record& record = m_RecordsByAverageTime.emplace_back();
            record.Name = name;
            record.AverageTimeInMilliseconds = data.Time / (float)data.RecordsCount;
        }

        std::sort(m_RecordsByAverageTime.rbegin(), m_RecordsByAverageTime.rend(), [](const Record& a, const Record& b) -> bool
        {
            return a.AverageTimeInMilliseconds < b.AverageTimeInMilliseconds;
        });
    }
}
