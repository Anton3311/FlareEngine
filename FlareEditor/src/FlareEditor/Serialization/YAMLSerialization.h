#pragma once

#include "FlareCore/Serialization/SerializationStream.h"

#include <yaml-cpp/yaml.h>

namespace Flare
{
    class YAMLSerializer : public SerializationStream
    {
    public:
        YAMLSerializer(YAML::Emitter& emitter);
    public:
        void PropertyKey(std::string_view key) override;
        void SerializeInt32(SerializationValue<int32_t> value) override;
        void SerializeUInt32(SerializationValue<uint32_t> value) override;
        void SerializeFloat(SerializationValue<float> value) override;
        void SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount) override;
        void SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount) override;
        void SerializeString(SerializationValue<std::string> value) override;
        void BeginArray() override;
        void EndArray() override;
        void BeginObject(const SerializableObjectDescriptor* descriptor) override;
        void EndObject() override;
        void SerializeObject(const SerializableObjectDescriptor& descriptor, void* objectData) override;
    public:
        YAML::Emitter& m_Emitter;
        bool m_ObjectSerializationStarted;
        bool m_MapStarted;
    };

    class YAMLDeserializer : public SerializationStream
    {
    public:
        YAMLDeserializer(const YAML::Node& root);

        void PropertyKey(std::string_view key) override;
        void SerializeInt32(SerializationValue<int32_t> value) override;
        void SerializeUInt32(SerializationValue<uint32_t> value) override;
        void SerializeFloat(SerializationValue<float> value) override;
        void SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount) override;
        void SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount) override;
        void SerializeString(SerializationValue<std::string> value) override;
        void BeginArray() override;
        void EndArray() override;
        void BeginObject(const SerializableObjectDescriptor* descriptor) override;
        void EndObject() override;
        void SerializeObject(const SerializableObjectDescriptor& descriptor, void* objectData) override;
    private:
        void BeginNode();
        void EndNode();

        inline YAML::Node& CurrentNode()
        {
            FLARE_CORE_ASSERT(m_NodesStack.size() > 0);
            return m_NodesStack.back();
        }
    private:
        const YAML::Node& m_Root;

        size_t m_CurrentNodeDepth;
        size_t m_SkippedNodeDepth;
        bool m_Skip;
        std::string m_CurrentPropertyKey;
        std::vector<YAML::Node> m_NodesStack;
    };
}