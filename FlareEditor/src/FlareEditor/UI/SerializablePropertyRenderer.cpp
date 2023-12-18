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
        if (!CurrentTreeNodeState())
            return;

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
        if (!CurrentTreeNodeState())
            return;
        EditorGUI::UIntPropertyField(m_CurrentPropertyName.data(), value.Values[0]);
    }

    void SerializablePropertyRenderer::SerializeFloat(SerializationValue<float> value)
    {
        if (!CurrentTreeNodeState())
            return;
        EditorGUI::FloatPropertyField(m_CurrentPropertyName.data(), value.Values[0]);
    }

    void SerializablePropertyRenderer::SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount)
    {
        size_t elementsCount = value.Values.GetSize() / (size_t)componentsCount;
        FLARE_CORE_ASSERT(!m_IsArray && elementsCount == 1 || m_IsArray);

        if (!CurrentTreeNodeState())
            return;

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
        size_t elementsCount = value.Values.GetSize() / (size_t)componentsCount;
        FLARE_CORE_ASSERT(!m_IsArray && elementsCount == 1 || m_IsArray);

        if (!CurrentTreeNodeState())
            return;

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

    void SerializablePropertyRenderer::BeginArray()
    {
        EditorGUI::EndPropertyGrid();
        
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
        bool open = ImGui::TreeNodeEx(m_CurrentPropertyName.data(), flags);

        m_TreeNodeStates.push_back(open);
        if (open)
        {
            EditorGUI::BeginPropertyGrid();
        }
    }

    void SerializablePropertyRenderer::EndArray()
    {
        FLARE_CORE_ASSERT(m_TreeNodeStates.size() > 0);

        if (m_TreeNodeStates.back())
        {
            EditorGUI::EndPropertyGrid();
            ImGui::TreePop();
        }

        m_TreeNodeStates.pop_back();
        EditorGUI::BeginPropertyGrid();
    }

    void SerializablePropertyRenderer::BeginObject(const SerializableObjectDescriptor* descriptor)
    {
    }

    void SerializablePropertyRenderer::EndObject()
    {
    }

    bool SerializablePropertyRenderer::CurrentTreeNodeState()
    {
        if (m_TreeNodeStates.size() > 0)
        {
            return m_TreeNodeStates.back();
        }

        return true;
    }
}
