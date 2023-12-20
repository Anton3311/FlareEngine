#include "YAMLSerialization.h"

#include "Flare/Serialization/Serialization.h"

namespace Flare
{
    YAMLSerializer::YAMLSerializer(YAML::Emitter& emitter)
        : m_Emitter(emitter) {}

    void YAMLSerializer::PropertyKey(std::string_view key)
    {
        m_Emitter << YAML::Key << std::string(key);
    }

    void YAMLSerializer::SerializeInt32(SerializationValue<int32_t> value)
    {
        for (size_t i = 0; i < value.Values.GetSize(); i++)
        {
            m_Emitter << YAML::Value << value.Values[i];
        }
    }

    void YAMLSerializer::SerializeUInt32(SerializationValue<uint32_t> value)
    {
        for (size_t i = 0; i < value.Values.GetSize(); i++)
        {
            m_Emitter << YAML::Value << value.Values[i];
        }
    }

    void YAMLSerializer::SerializeFloat(SerializationValue<float> value)
    {
        for (size_t i = 0; i < value.Values.GetSize(); i++)
        {
            m_Emitter << YAML::Value << value.Values[i];
        }
    }

    void YAMLSerializer::SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount)
    {
        for (size_t i = 0; i < value.Values.GetSize(); i += (size_t)componentsCount)
        {
            m_Emitter << YAML::Value;
            float* vectorValues = &value.Values[i];
            switch (componentsCount)
            {
            case 1:
                m_Emitter << *vectorValues;
                break;
            case 2:
                m_Emitter << glm::vec2(vectorValues[0], vectorValues[1]);
                break;
            case 3:
                m_Emitter << glm::vec3(vectorValues[0], vectorValues[1], vectorValues[2]);
                break;
            case 4:
                m_Emitter << glm::vec4(vectorValues[0], vectorValues[1], vectorValues[2], vectorValues[3]);
                break;
            }
        }
    }

    void YAMLSerializer::SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount)
    {
        for (size_t i = 0; i < value.Values.GetSize(); i += (size_t)componentsCount)
        {
            m_Emitter << YAML::Value;
            int32_t* vectorValues = &value.Values[i];
            switch (componentsCount)
            {
            case 1:
                m_Emitter << *vectorValues;
                break;
            case 2:
                m_Emitter << glm::ivec2(vectorValues[0], vectorValues[1]);
                break;
            case 3:
                m_Emitter << glm::ivec3(vectorValues[0], vectorValues[1], vectorValues[2]);
                break;
            case 4:
                m_Emitter << glm::ivec4(vectorValues[0], vectorValues[1], vectorValues[2], vectorValues[3]);
                break;
            }
        }
    }

    void YAMLSerializer::SerializeString(SerializationValue<std::string> value)
    {
        for (size_t i = 0; i < value.Values.GetSize(); i++)
        {
            m_Emitter << YAML::Value << value.Values[i];
        }
    }

    void YAMLSerializer::BeginArray()
    {
        m_Emitter << YAML::BeginSeq;
    }

    void YAMLSerializer::EndArray()
    {
        m_Emitter << YAML::EndSeq;
    }

    void YAMLSerializer::BeginObject(const SerializableObjectDescriptor* descriptor)
    {
        m_Emitter << YAML::BeginMap;
    }

    void YAMLSerializer::EndObject()
    {
        m_Emitter << YAML::EndMap;
    }

    // YAML Deserializer

    YAMLDeserializer::YAMLDeserializer(const YAML::Node& root)
        : m_Root(root), m_Skip(false), m_SkippedNodeDepth(SIZE_MAX), m_CurrentNodeDepth(0)
    {
        m_NodesStack.push_back(m_Root);
    }

    void YAMLDeserializer::PropertyKey(std::string_view key)
    {
        m_CurrentPropertyKey = key;
    }

    template<typename T>
    void DeserializeValue(SerializationValue<T>& value, const YAML::Node& currentNode, const std::string& currentPropertyName)
    {
        if (value.IsArray)
        {
            size_t index = 0;
            for (const YAML::Node& node : currentNode)
            {
                if (index >= value.Values.GetSize())
                    break;

                value.Values[index] = node.as<T>();
                index++;
            }
        }
        else
        {
            if (YAML::Node node = currentNode[currentPropertyName])
                value.Values[0] = node.as<T>();
        }
    }

    void YAMLDeserializer::SerializeInt32(SerializationValue<int32_t> value)
    {
        if (m_Skip)
            return;

        DeserializeValue(value, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeUInt32(SerializationValue<uint32_t> value)
    {
        if (m_Skip)
            return;

        DeserializeValue(value, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeFloat(SerializationValue<float> value)
    {
        if (m_Skip)
            return;

        DeserializeValue(value, CurrentNode(), m_CurrentPropertyKey);
    }

    template<typename T>
    static void DeserializeSingleVector(T* destination, const YAML::Node& node, size_t componentsCount)
    {
        switch (componentsCount)
        {
        case 1:
            *destination = node.as<T>();
            break;
        case 2:
        {
            auto vector = node.as<glm::vec<2, T>>();
            std::memcpy(destination, &vector, sizeof(vector));
            break;
        }
        case 3:
        {
            auto vector = node.as<glm::vec<3, T>>();
            std::memcpy(destination, &vector, sizeof(vector));
            break;
        }
        case 4:
        {
            auto vector = node.as<glm::vec<4, T>>();
            std::memcpy(destination, &vector, sizeof(vector));
            break;
        }
        }
    }

    template<typename T>
    static void DeserializeVector(SerializationValue<T>& value, uint32_t componentsCount, YAML::Node& currentNode, const std::string& currentKey)
    {
        if (!value.IsArray)
        {
            const auto& node = currentNode[currentKey];
            DeserializeSingleVector<T>(&value.Values[0], node, (size_t)componentsCount);
            return;
        }

        size_t vectorIndex = 0;
        for (const YAML::Node& node : currentNode)
        {
            DeserializeSingleVector<T>(&value.Values[vectorIndex], node, (size_t)componentsCount);
            vectorIndex += (size_t)componentsCount;

            if (vectorIndex >= value.Values.GetSize())
                break;
        }
    }

    void YAMLDeserializer::SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount)
    {
        if (m_Skip)
            return;

        DeserializeVector<float>(value, componentsCount, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount)
    {
        if (m_Skip)
            return;

        DeserializeVector<int32_t>(value, componentsCount, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeString(SerializationValue<std::string> value)
    {
        DeserializeValue<std::string>(value, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::BeginArray()
    {
        BeginNode();
    }

    void YAMLDeserializer::EndArray()
    {
        EndNode();
    }

    void YAMLDeserializer::BeginObject(const SerializableObjectDescriptor* descriptor)
    {
        BeginNode();
    }

    void YAMLDeserializer::EndObject()
    {
        EndNode();
    }

    void YAMLDeserializer::BeginNode()
    {
        m_CurrentNodeDepth++;
        if (m_Skip)
            return;

        if (YAML::Node node = CurrentNode()[m_CurrentPropertyKey])
        {
            m_NodesStack.push_back(node);
        }
        else if (!m_Skip)
        {
            m_SkippedNodeDepth = m_CurrentNodeDepth;
            m_Skip = true;
        }
    }

    void YAMLDeserializer::EndNode()
    {
        m_CurrentNodeDepth--;
        if (m_SkippedNodeDepth == m_CurrentNodeDepth)
        {
            m_SkippedNodeDepth = SIZE_MAX;
            m_Skip = false;
        }

        if (m_Skip)
            return;

        m_NodesStack.pop_back();
    }
}
