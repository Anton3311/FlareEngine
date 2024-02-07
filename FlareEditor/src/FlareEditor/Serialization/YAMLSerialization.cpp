#include "YAMLSerialization.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Serialization/Serialization.h"

#include "FlareEditor/Serialization/SerializationId.h"

namespace Flare
{
    YAMLSerializer::YAMLSerializer(YAML::Emitter& emitter, const World* world)
        : m_Emitter(emitter), m_MapStarted(false), m_ObjectSerializationStarted(false), m_World(world) {}

    void YAMLSerializer::PropertyKey(std::string_view key)
    {
        if (!m_MapStarted && m_ObjectSerializationStarted)
        {
            m_Emitter << YAML::BeginMap;
            m_MapStarted = true;
        }

        m_Emitter << YAML::Key << std::string(key);
    }

    SerializationStream::DynamicArrayAction YAMLSerializer::SerializeDynamicArraySize(size_t& size)
    {
        return DynamicArrayAction::None;
    }

    void YAMLSerializer::SerializeInt(SerializationValue<uint8_t> intValues, SerializableIntType type)
    {
        if (intValues.IsArray)
            m_Emitter << YAML::BeginSeq;

        size_t intSize = SizeOfSerializableIntType(type);
        for (size_t i = 0; i < intValues.Values.GetSize(); i += intSize)
        {
            m_Emitter << YAML::Value;
            
#define SERIALIZE_INT(intType, serializableIntType)                           \
            case SerializableIntType::serializableIntType:                    \
                m_Emitter << reinterpret_cast<intType&>(intValues.Values[i]); \
                break;

            switch (type)
            {
            case SerializableIntType::Int8:
                m_Emitter << (int16_t)reinterpret_cast<int8_t&>(intValues.Values[i]);
                break;
            case SerializableIntType::UInt8:
                m_Emitter << (uint16_t)reinterpret_cast<uint8_t&>(intValues.Values[i]);
                break;

                SERIALIZE_INT(int16_t, Int16);
                SERIALIZE_INT(uint16_t, UInt16);
                SERIALIZE_INT(int32_t, Int32);
                SERIALIZE_INT(uint32_t, UInt32);
                SERIALIZE_INT(int64_t, Int64);
                SERIALIZE_INT(uint64_t, UInt64);
            }

#undef SERIALIZE_INT
        }

        if (intValues.IsArray)
            m_Emitter << YAML::EndSeq;
    }

    template<typename T>
    static void SerializeValue(YAML::Emitter& emitter, SerializationValue<T>& value)
    {
        if (value.IsArray)
            emitter << YAML::BeginSeq;

        for (size_t i = 0; i < value.Values.GetSize(); i++)
            emitter << YAML::Value << value.Values[i];

        if (value.IsArray)
            emitter << YAML::EndSeq;
    }

    void YAMLSerializer::SerializeBool(SerializationValue<bool> value)
    {
        SerializeValue(m_Emitter, value);
    }

    void YAMLSerializer::SerializeFloat(SerializationValue<float> value)
    {
        SerializeValue(m_Emitter, value);
    }

    void YAMLSerializer::SerializeUUID(SerializationValue<UUID> uuids)
    {
        SerializeValue(m_Emitter, uuids);
    }

    void YAMLSerializer::SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount)
    {
        if (value.IsArray)
            m_Emitter << YAML::BeginSeq;

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

        if (value.IsArray)
            m_Emitter << YAML::EndSeq;
    }

    void YAMLSerializer::SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount)
    {
        if (value.IsArray)
            m_Emitter << YAML::BeginSeq;

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

        if (value.IsArray)
            m_Emitter << YAML::EndSeq;
    }

    void YAMLSerializer::SerializeString(SerializationValue<std::string> value)
    {
        if (value.IsArray)
            m_Emitter << YAML::BeginSeq;

        for (size_t i = 0; i < value.Values.GetSize(); i++)
            m_Emitter << YAML::Value << std::string(value.Values[i]);

        if (value.IsArray)
            m_Emitter << YAML::EndSeq;
    }

    template<typename T>
    static void SerializeObjects(YAML::Emitter& emitter, bool isArray, T* values, size_t arraySize)
    {
        if (isArray)
        {
            emitter << YAML::BeginSeq;

            for (size_t i = 0; i < arraySize; i++)
                emitter << YAML::Value << values[i];

            emitter << YAML::EndSeq;
        }
        else
        {
            emitter << YAML::Value << values[0];
        }
    }

    static void SerializeEntityId(YAML::Emitter& emitter, const World& world, Entity id)
    {
        if (world.IsEntityAlive(id))
        {
            const SerializationId* serializationId = world.TryGetEntityComponent<const SerializationId>(id);
            if (serializationId)
            {
                emitter << YAML::Value << serializationId->Id;
                return;
            }
        }

        emitter << YAML::Value << 0;
        return;
    }

    void YAMLSerializer::SerializeObject(const SerializableObjectDescriptor& descriptor, void* objectData, bool isArray, size_t arraySize)
    {
        if (&descriptor == &FLARE_SERIALIZATION_DESCRIPTOR_OF(AssetHandle))
        {
            SerializeObjects<AssetHandle>(m_Emitter, isArray, (AssetHandle*)objectData, arraySize);
            return;
        }

        if (&descriptor == &FLARE_SERIALIZATION_DESCRIPTOR_OF(Entity))
        {
            if (m_World == nullptr)
                return;

            Entity* entityIds = (Entity*)objectData;
            if (isArray)
            {
                m_Emitter << YAML::BeginSeq;
                for (size_t i = 0; i < arraySize; i++)
                    SerializeEntityId(m_Emitter, *m_World, entityIds[i]);
                m_Emitter << YAML::EndSeq;
            }
            else
            {
                SerializeEntityId(m_Emitter, *m_World, entityIds[0]);
            }

            return;
        }

        bool previousObjectSerializationState = m_ObjectSerializationStarted;
        bool previousState = m_MapStarted;

        auto serializeSingleObject = [=](void* object)
        {
            m_ObjectSerializationStarted = true;
            m_MapStarted = false;

            // NOTE: Map starts when the serializer recieves a property with a key,
            //       this prevents YAML-cpp from generating an invalid one when an
            //       object serializes it's properties without any keys
            descriptor.Callback(object, *this);

            // NOTE: 'm_MapStarted' whether YAML::BeginMap was emited which only happens when the first property of the object is being serialized
            //       if 'm_MapStarted' is false, it means that the object doens't have any serializable properties
            if (!m_MapStarted)
                m_Emitter << YAML::Block << YAML::BeginMap << YAML::EndMap;

            if (m_MapStarted)
            {
                m_Emitter << YAML::EndMap;
            }

            m_MapStarted = previousState;
            m_ObjectSerializationStarted = previousObjectSerializationState;
        };

        if (isArray)
        {
            m_Emitter << YAML::Value << YAML::BeginSeq;
            for (size_t i = 0; i < arraySize; i++)
                serializeSingleObject((uint8_t*)objectData + i * descriptor.Size);

            m_Emitter << YAML::EndSeq;
        }
        else
        {
            serializeSingleObject(objectData);
        }
    }

    void YAMLSerializer::SerializeReference(const SerializableObjectDescriptor& valueDescriptor, void* referenceData, void* valueData)
    {
        const AssetDescriptor* assetDescriptor = AssetDescriptor::FindBySerializationDescriptor(valueDescriptor);
        if (assetDescriptor)
        {
            FLARE_CORE_ASSERT(referenceData);
            Ref<Asset>& asset = *(Ref<Asset>*)referenceData;

            if (asset == nullptr)
                m_Emitter << YAML::Value << NULL_ASSET_HANDLE;
            else if (AssetManager::IsAssetHandleValid(asset->Handle))
                m_Emitter << YAML::Value << asset->Handle;
            else
                m_Emitter << YAML::Value << NULL_ASSET_HANDLE;
        }
        else
        {
            FLARE_CORE_WARN("YAMLSerializer: Reference serialization not supported");
            m_Emitter << YAML::Value << YAML::Null;
        }
    }

    // YAML Deserializer

    YAMLDeserializer::YAMLDeserializer(const YAML::Node& root, std::unordered_map<UUID, Entity>* serializationIdToECSId)
        : m_Root(root), m_SerializationIdToECSId(serializationIdToECSId)
    {
        m_NodesStack.push_back(m_Root);
    }

    void YAMLDeserializer::PropertyKey(std::string_view key)
    {
        m_CurrentPropertyKey = key;
    }

    SerializationStream::DynamicArrayAction YAMLDeserializer::SerializeDynamicArraySize(size_t& size)
    {
        size = CurrentNode().size();
        return DynamicArrayAction::Resize;
    }

    static void DeserializeIntValue(uint8_t* outValue, const YAML::Node& node, SerializableIntType type)
    {
#define DESERIALIZE_INT(intType, serializableIntType)       \
            case SerializableIntType::serializableIntType:  \
                *(intType*)(outValue) = node.as<intType>(); \
                break;

        switch (type)
        {
        case SerializableIntType::Int8:
            (*(int8_t*)outValue) = (int8_t)node.as<int16_t>();
            break;
        case SerializableIntType::UInt8:
            (*(uint8_t*)outValue) = (uint8_t)node.as<uint16_t>();
            break;

            DESERIALIZE_INT(int16_t, Int16);
            DESERIALIZE_INT(uint16_t, UInt16);
            DESERIALIZE_INT(int32_t, Int32);
            DESERIALIZE_INT(uint32_t, UInt32);
            DESERIALIZE_INT(int64_t, Int64);
            DESERIALIZE_INT(uint64_t, UInt64);
        }

#undef DESERIALIZE_INT
    }

    void YAMLDeserializer::SerializeInt(SerializationValue<uint8_t> intValues, SerializableIntType type)
    {
        size_t intSize = SizeOfSerializableIntType(type);
        if (intValues.IsArray)
        {
            size_t index = 0;
            for (const YAML::Node& node : CurrentNode())
            {
                if (index >= intValues.Values.GetSize())
                    break;
                
                DeserializeIntValue(&intValues.Values[index], node, type);
                index += intSize;
            }
        }
        else
        {
            if (YAML::Node node = CurrentNode()[m_CurrentPropertyKey])
                DeserializeIntValue(intValues.Values.GetData(), node, type);
        }
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

    void YAMLDeserializer::SerializeBool(SerializationValue<bool> value)
    {
        DeserializeValue(value, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeFloat(SerializationValue<float> value)
    {
        DeserializeValue(value, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeUUID(SerializationValue<UUID> uuids)
    {
        DeserializeValue(uuids, CurrentNode(), m_CurrentPropertyKey);
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
        DeserializeVector<float>(value, componentsCount, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount)
    {
        DeserializeVector<int32_t>(value, componentsCount, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeString(SerializationValue<std::string> value)
    {
        DeserializeValue<std::string>(value, CurrentNode(), m_CurrentPropertyKey);
    }

    void YAMLDeserializer::SerializeObject(const SerializableObjectDescriptor& descriptor, void* objectData, bool isArray, size_t arraySize)
    {
        try
        {
            if (&descriptor == &FLARE_SERIALIZATION_DESCRIPTOR_OF(AssetHandle))
            {
                if (YAML::Node handleNode = CurrentNode()[m_CurrentPropertyKey])
                    (*(AssetHandle*)objectData) = handleNode.as<AssetHandle>();

                return;
            }

            if (&descriptor == &FLARE_SERIALIZATION_DESCRIPTOR_OF(Entity))
            {
                if (m_SerializationIdToECSId == nullptr)
                    return;

                Entity* entities = (Entity*)objectData;
                if (isArray)
                {
                    YAML::Node node = CurrentNode();
                    size_t index = 0;

                    for (YAML::Node itemNode : node)
                    {
                        if (index >= arraySize)
                            break;

                        UUID serializationId = itemNode.as<UUID>();

                        auto it = m_SerializationIdToECSId->find(serializationId);
                        if (it != m_SerializationIdToECSId->end())
                            entities[index] = it->second;
                        else
                            entities[index] = Entity();

                        index++;
                    }
                }
                else
                {
                    YAML::Node node = CurrentNode()[m_CurrentPropertyKey];
                    UUID serializationId = node.as<UUID>();

                    auto it = m_SerializationIdToECSId->find(serializationId);
                    if (it != m_SerializationIdToECSId->end())
                    {
                        entities[0] = it->second;
                        return;
                    }

                    entities[0] = Entity();
                }

                return;
            }

            if (isArray)
            {
                size_t index = 0;
                for (YAML::Node itemNode : CurrentNode())
                {
                    if (index >= arraySize)
                        break;

                    m_NodesStack.push_back(itemNode);
                    descriptor.Callback((uint8_t*)objectData + index * descriptor.Size, *this);
                    m_NodesStack.pop_back();

                    index++;
                }
            }
            else
            {
                YAML::Node objectNode = m_CurrentPropertyKey.empty() ? CurrentNode() : CurrentNode()[m_CurrentPropertyKey];
                if (objectNode)
                {
                    m_NodesStack.push_back(objectNode);
                    descriptor.Callback(objectData, *this);
                    m_NodesStack.pop_back();
                }
            }
        }
        catch (YAML::BadConversion& e)
        {
            FLARE_CORE_ERROR("Failed to deserialize object Line: {} Col: {} Erorr: {}", e.mark.line, e.mark.column, e.what());
        }
        catch (std::exception& e)
        {
            FLARE_CORE_ERROR("Failed to deserialize object: {}", e.what());
        }
    }

    void YAMLDeserializer::SerializeReference(const SerializableObjectDescriptor& valueDescriptor, void* referenceData, void* valueData)
    {
        const AssetDescriptor* assetDescriptor = AssetDescriptor::FindBySerializationDescriptor(valueDescriptor);
        if (assetDescriptor)
        {
            FLARE_CORE_ASSERT(referenceData);
            Ref<Asset>& asset = *(Ref<Asset>*)referenceData;

            YAML::Node node = CurrentNode()[m_CurrentPropertyKey];
            if (node)
            {
                AssetHandle handle = node.as<AssetHandle>();
                if (!AssetManager::IsAssetHandleValid(handle))
                    asset = nullptr;
                else
                {
                    Ref<Asset> deserializedAsset = AssetManager::GetRawAsset(handle);
                    if (&deserializedAsset->GetDescriptor() == assetDescriptor)
                        asset = deserializedAsset;
                    else
                        asset = nullptr;
                }
            }
        }
        else
        {
            FLARE_CORE_WARN("YAMLDeserializer: Reference deserialization not supported");
        }
    }
}
