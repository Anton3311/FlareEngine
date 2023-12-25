#include "SerializablePropertyRenderer.h"

#include "FlareEditor/UI/EditorGUI.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
    SerializablePropertyRenderer::SerializablePropertyRenderer()
        : m_CurrentState({ false })
    {
    }

    void SerializablePropertyRenderer::PropertyKey(std::string_view key)
    {
        m_CurrentPropertyName = key;
    }

    void SerializablePropertyRenderer::SerializeInt32(SerializationValue<int32_t> value)
    {
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

    void SerializablePropertyRenderer::SerializeBool(SerializationValue<bool> value)
    {
        BeginPropertiesGridIfNeeded();

        if (!value.IsArray)
            EditorGUI::PropertyName(m_CurrentPropertyName.data());

        for (size_t i = 0; i < value.Values.GetSize(); i++)
        {
            if (value.IsArray)
                EditorGUI::PropertyIndex(i);

            ImGui::PushID(&value.Values[i]);
            ImGui::Checkbox("", &value.Values[i]);
            ImGui::PopID();
        }
    }

    void SerializablePropertyRenderer::SerializeFloat(SerializationValue<float> value)
    {
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
                if (HAS_BIT(value.Flags, SerializationValueFlags::Color))
                    ImGui::ColorEdit3("", &value.Values[i], ImGuiColorEditFlags_Float);
                else
                    ImGui::DragFloat3("", &value.Values[i]);
                break;
            case 4:
                if (HAS_BIT(value.Flags, SerializationValueFlags::Color))
                    ImGui::ColorEdit4("", &value.Values[i], ImGuiColorEditFlags_Float);
                else
                    ImGui::DragFloat4("", &value.Values[i]);
                break;
            }

            ImGui::PopID();
        }
    }

    void SerializablePropertyRenderer::SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount)
    {
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

    void SerializablePropertyRenderer::SerializeObject(const SerializableObjectDescriptor& descriptor, void* objectData)
    {
        if (m_CurrentState.GridStarted)
        {
            EditorGUI::EndPropertyGrid();
            m_CurrentState.GridStarted = false;
        }

        if (&descriptor == &FLARE_SERIALIZATION_DESCRIPTOR_OF(AssetHandle))
        {
            RenderAssetField(*reinterpret_cast<AssetHandle*>(objectData));
            return;
        }

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
        bool expanded = ImGui::TreeNodeEx(m_CurrentPropertyName.data(), flags);

        if (expanded)
        {
            PropertiesTreeState previousState = m_CurrentState;
            m_CurrentState.GridStarted = false;

            descriptor.Callback(objectData, *this);

            if (m_CurrentState.GridStarted)
                EditorGUI::EndPropertyGrid();

            ImGui::TreePop();
            m_CurrentState = previousState;
        }
    }

    void SerializablePropertyRenderer::RenderAssetField(AssetHandle& handle)
    {
        EditorGUI::AssetField(handle);
    }

    void SerializablePropertyRenderer::BeginPropertiesGridIfNeeded()
    {
        if (!m_CurrentState.GridStarted)
        {
            EditorGUI::BeginPropertyGrid();
            m_CurrentState.GridStarted = true;
        }
    }
}
