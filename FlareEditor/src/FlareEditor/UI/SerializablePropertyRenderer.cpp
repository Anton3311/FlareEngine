#include "SerializablePropertyRenderer.h"

#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
    void SerializablePropertyRenderer::PropertyKey(std::string_view key)
    {
        m_CurrentPropertyName = key;
    }

    void SerializablePropertyRenderer::SerializeInt32(SerializationValue<int32_t> value)
    {
        if (!CurrentTreeNodeState().Expanded)
            return;

        BeginPropertiesGridIfNeeded();

        if (!value.IsArray)
            EditorGUI::PropertyName(m_CurrentPropertyName.data());

        for (size_t i = 0; i < value.Values.GetSize(); i++)
        {
            if (value.IsArray)
                EditorGUI::PropertyIndex(i);

            ImGui::PushID(&value.Values[i]);
            ImGui::DragInt("", &value.Values[i]);
            ImGui::PopID();
        }
    }

    void SerializablePropertyRenderer::SerializeUInt32(SerializationValue<uint32_t> value)
    {
        if (!CurrentTreeNodeState().Expanded)
            return;

        BeginPropertiesGridIfNeeded();

        if (!value.IsArray)
            EditorGUI::PropertyName(m_CurrentPropertyName.data());

        for (size_t i = 0; i < value.Values.GetSize(); i++)
        {
            if (value.IsArray)
                EditorGUI::PropertyIndex(i);

            ImGui::PushID(&value.Values[i]);
            ImGui::DragScalar("", ImGuiDataType_U32, &value.Values[i]);
            ImGui::PopID();
        }
    }

    void SerializablePropertyRenderer::SerializeFloat(SerializationValue<float> value)
    {
        if (!CurrentTreeNodeState().Expanded)
            return;

        BeginPropertiesGridIfNeeded();
        
        if (!value.IsArray)
            EditorGUI::PropertyName(m_CurrentPropertyName.data());

        for (size_t i = 0; i < value.Values.GetSize(); i++)
        {
            if (value.IsArray)
                EditorGUI::PropertyIndex(i);

            ImGui::PushID(&value.Values[i]);
            ImGui::DragFloat("", &value.Values[i]);
            ImGui::PopID();
        }
    }

    void SerializablePropertyRenderer::SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount)
    {
        if (!CurrentTreeNodeState().Expanded)
            return;

        BeginPropertiesGridIfNeeded();

        if (!value.IsArray)
            EditorGUI::PropertyName(m_CurrentPropertyName.data());

        for (size_t i = 0; i < value.Values.GetSize(); i += (size_t)componentsCount)
        {
            if (value.IsArray)
                EditorGUI::PropertyIndex(i / (size_t)componentsCount);

            ImGui::PushID(&value.Values[i]);
            switch (componentsCount)
            {
            case 1:
                ImGui::DragFloat("", &value.Values[i]);
                break;
            case 2:
                ImGui::DragFloat2("", &value.Values[i]);
                break;
            case 3:
                ImGui::DragFloat3("", &value.Values[i]);
                break;
            case 4:
                ImGui::DragFloat4("", &value.Values[i]);
                break;
            }

            ImGui::PopID();
        }
    }

    void SerializablePropertyRenderer::SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount)
    {
        if (!CurrentTreeNodeState().Expanded)
            return;

        BeginPropertiesGridIfNeeded();

        if (!value.IsArray)
            EditorGUI::PropertyName(m_CurrentPropertyName.data());
        
        for (size_t i = 0; i < value.Values.GetSize(); i += (size_t)componentsCount)
        {
            if (value.IsArray)
                EditorGUI::PropertyIndex(i / (size_t)componentsCount);

            ImGui::PushID(&value.Values[i]);
            switch (componentsCount)
            {
            case 1:
                ImGui::DragInt("", &value.Values[i]);
                break;
            case 2:
                ImGui::DragInt2("", &value.Values[i]);
                break;
            case 3:
                ImGui::DragInt3("", &value.Values[i]);
                break;
            case 4:
                ImGui::DragInt4("", &value.Values[i]);
                break;
            }

            ImGui::PopID();
        }
    }

    void SerializablePropertyRenderer::SerializeString(SerializationValue<std::string> value)
    {
        if (!CurrentTreeNodeState().Expanded)
            return;

        BeginPropertiesGridIfNeeded();

        if (!value.IsArray)
            EditorGUI::PropertyName(m_CurrentPropertyName.data());

        for (size_t i = 0; i < value.Values.GetSize(); i++)
        {
            if (value.IsArray)
                EditorGUI::PropertyIndex(i);
            
            EditorGUI::TextField(value.Values[i]);
        }
    }

    void SerializablePropertyRenderer::BeginArray()
    {
        BeginTreeNode();
    }

    void SerializablePropertyRenderer::EndArray()
    {
        EndTreeNode();
    }

    void SerializablePropertyRenderer::BeginObject(const SerializableObjectDescriptor* descriptor)
    {
        BeginTreeNode();
    }

    void SerializablePropertyRenderer::EndObject()
    {
        EndTreeNode();
    }

    void SerializablePropertyRenderer::BeginTreeNode()
    {
        if (m_TreeNodeStates.size() > 0)
        {
            PropertiesTreeState& currentState = CurrentTreeNodeState();
            if (currentState.GridStarted)
            {
                EditorGUI::EndPropertyGrid();
                currentState.GridStarted = false;
            }
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
        bool expanded = false;
        if (m_TreeNodeStates.size() == 0 || CurrentTreeNodeState().Expanded)
        {
            expanded = ImGui::TreeNodeEx(m_CurrentPropertyName.data(), flags);
        }

        auto& state = m_TreeNodeStates.emplace_back();
        state.Expanded = expanded;
        state.GridStarted = false;
    }

    void SerializablePropertyRenderer::EndTreeNode()
    {
        PropertiesTreeState& state = CurrentTreeNodeState();

        if (state.GridStarted)
            EditorGUI::EndPropertyGrid();

        if (state.Expanded)
            ImGui::TreePop();

        m_TreeNodeStates.pop_back();
    }

    void SerializablePropertyRenderer::BeginPropertiesGridIfNeeded()
    {
        PropertiesTreeState& state = CurrentTreeNodeState();
        
        if (!state.GridStarted)
        {
            EditorGUI::BeginPropertyGrid();
            state.GridStarted = true;
        }
    }
}
