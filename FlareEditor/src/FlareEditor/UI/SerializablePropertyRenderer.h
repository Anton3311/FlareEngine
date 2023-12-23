#pragma once

#include "FlareCore/Serialization/SerializationStream.h"

#include <string_view>
#include <vector>

namespace Flare
{
    class SerializablePropertyRenderer : public SerializationStream
    {
    public:
        SerializablePropertyRenderer();

        void PropertyKey(std::string_view key) override;
        void SerializeInt32(SerializationValue<int32_t> value) override;
        void SerializeUInt32(SerializationValue<uint32_t> value) override;
        void SerializeFloat(SerializationValue<float> value) override;
        void SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount) override;
        void SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount) override;
        void SerializeString(SerializationValue<std::string> value) override;
        void SerializeObject(const SerializableObjectDescriptor& descriptor, void* objectData) override;
    private:
        struct PropertiesTreeState
        {
            bool GridStarted;
        };

        void BeginPropertiesGridIfNeeded();
    private:
        PropertiesTreeState m_CurrentState;
        std::string_view m_CurrentPropertyName;
    };
}