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
        : m_Root(root)
    {
        m_NodesStack.push_back(m_Root);
    }

    void YAMLDeserializer::PropertyKey(std::string_view key)
    {
        m_CurrentPropertyKey = key;
    }

    void YAMLDeserializer::SerializeInt32(SerializationValue<int32_t> value)
    {
        if (value.IsArray)
        {
            size_t index = 0;
            for (const YAML::Node& node : CurrentNode())
            {
                if (index > value.Values.GetSize())
                    break;

                value.Values[index] = node.as<int32_t>();
            }
        }
        else
        {
            if (YAML::Node node = CurrentNode()[m_CurrentPropertyKey])
                value.Values[0] = node.as<int32_t>();
        }
    }

    void YAMLDeserializer::SerializeUInt32(SerializationValue<uint32_t> value)
    {
        if (value.IsArray)
        {
            size_t index = 0;
            for (const YAML::Node& node : CurrentNode())
            {
                if (index > value.Values.GetSize())
                    break;

                value.Values[index] = node.as<uint32_t>();
            }
        }
        else
        {
            if (YAML::Node node = CurrentNode()[m_CurrentPropertyKey])
                value.Values[0] = node.as<uint32_t>();
        }
    }

    void YAMLDeserializer::SerializeFloat(SerializationValue<float> value)
    {
        if (value.IsArray)
        {
            size_t index = 0;
            for (const YAML::Node& node : CurrentNode())
            {
                if (index > value.Values.GetSize())
                    break;

                value.Values[index] = node.as<float>();
            }
        }
        else
        {
            if (YAML::Node node = CurrentNode()[m_CurrentPropertyKey])
                value.Values[0] = node.as<float>();
        }
    }

    template<typename T>
    static void SerializeSingleVector(T* destination, const YAML::Node& node, size_t componentsCount)
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
    static void SerializeVector(SerializationValue<T>& value, uint32_t componentsCount, YAML::Node& currentNode, const std::string& currentKey)
    {
        if (!value.IsArray)
        {
            const auto& node = currentNode[currentKey];
            SerializeSingleVector<T>(&value.Values[0], node, (size_t)componentsCount);
            return;
        }

        size_t vectorIndex = 0;
        for (const YAML::Node& node : currentNode)
        {
            SerializeSingleVector<T>(&value.Values[vectorIndex], node, (size_t)componentsCount);
            vectorIndex += (size_t)componentsCount;
        }
    }

    void YAMLDeserializer::SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount)
    {
        SerializeVector<float>(value, componentsCount, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount)
    {
        SerializeVector<int32_t>(value, componentsCount, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::BeginArray()
    {
        if (YAML::Node node = CurrentNode()[m_CurrentPropertyKey])
        {
            m_NodesStack.push_back(node);
        }
        else
        {
            // TODO: skip parsing this object
        }
    }

    void YAMLDeserializer::EndArray()
    {
        m_NodesStack.pop_back();
    }

    void YAMLDeserializer::BeginObject(const SerializableObjectDescriptor* descriptor)
    {
        if (YAML::Node node = CurrentNode()[m_CurrentPropertyKey])
        {
            m_NodesStack.push_back(node);
        }
        else
        {
            // TODO: skip parsing this object
        }
    }

    void YAMLDeserializer::EndObject()
    {
        m_NodesStack.pop_back();
    }
}
