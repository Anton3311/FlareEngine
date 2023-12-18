#pragma once

#include "FlareCore/Serialization/SerializationStream.h"

#include <string_view>
#include <vector>

namespace Flare
{
    class SerializablePropertyRenderer : public SerializationStreamBase
    {
    public:
        void PropertyKey(std::string_view key) override;
        void SerializeInt32(SerializationValue<int32_t> value) override;
        void SerializeUInt32(SerializationValue<uint32_t> value) override;
        void SerializeFloat(SerializationValue<float> value) override;
        void SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount) override;
        void SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount) override;
        void BeginArray() override;
        void EndArray() override;
        void BeginObject(const SerializableObjectDescriptor* descriptor) override;
        void EndObject() override;
    private:
        struct PropertiesTreeState
        {
            bool GridStarted;
            bool Expanded;
        };

        void BeginTreeNode();
        void EndTreeNode();

        void BeginPropertiesGridIfNeeded();

        inline PropertiesTreeState& CurrentTreeNodeState()
        {
            FLARE_CORE_ASSERT(m_TreeNodeStates.size() > 0);
            return m_TreeNodeStates.back();
        }
    private:
        std::vector<PropertiesTreeState> m_TreeNodeStates;
        std::string_view m_CurrentPropertyName;
    };
}