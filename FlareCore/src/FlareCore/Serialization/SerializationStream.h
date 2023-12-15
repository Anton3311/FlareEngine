#pragma once

#include "FlareCore/Collections/Span.h"

#include <stdint.h>
#include <string_view>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Flare
{
    template<typename T>
    struct SerializationValue
    {
        SerializationValue(T& value)
            : Values(Span(value)), IsArray(false) {}

        SerializationValue(const Span<T>& values)
            : Values(values), IsArray(true) {}

        Span<T> Values;
        bool IsArray;
    };

    class SerializableObjectDescriptor;

    class SerializationStreamBase
    {
    public:
        virtual void PropertyKey(std::string_view key) = 0;

        virtual void SerializeInt32(SerializationValue<int32_t> value) = 0;
        virtual void SerializeUInt32(SerializationValue<uint32_t> value) = 0;

        virtual void SerializeFloat(SerializationValue<float> value) = 0;

        virtual void SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount) = 0;
        virtual void SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount) = 0;

        virtual void BeginArray(std::string_view key) = 0;
        virtual void EndArray() = 0;

        virtual void BeginObject(std::string_view key, const SerializableObjectDescriptor* descriptor) = 0;
        virtual void EndObject() = 0;

        virtual void BeginArrayElementObject(const SerializableObjectDescriptor* descriptor) = 0;
        virtual void EndArratElementObject() = 0;
    };

    class SerializationStream;

    template<typename T>
    struct TypeSerializer
    {
        void OnSerialize(T& value, SerializationStream& stream) {}
    };

    template<typename T>
    struct SerializationDescriptorOf
    {
        constexpr const SerializableObjectDescriptor* Descriptor()
        {
            return &T::_SerializationDescriptor;
        }
    };

    class SerializationStream
    {
    public:
        SerializationStream(SerializationStreamBase& stream)
            : m_Stream(stream) {}

        template<typename T>
        inline void Serialize(std::string_view key, SerializationValue<T> value)
        {
            TypeSerializer<T> serializer;
            const SerializableObjectDescriptor* descriptor = SerializationDescriptorOf<T>().Descriptor();
            if (value.IsArray)
            {
                m_Stream.BeginArray(key);
                for (size_t i = 0; i < value.Values.GetSize(); i++)
                {
                    m_Stream.BeginArrayElementObject(descriptor);
                    serializer.OnSerialize(value.Values[i], *this);
                    m_Stream.EndArratElementObject();
                }
                m_Stream.EndArray();
            }
            else
            {
                m_Stream.BeginObject(key, descriptor);
                serializer.OnSerialize(value.Values[0], *this);
                m_Stream.EndObject();
            }
        }

        inline SerializationStreamBase& GetInternalStream() { return m_Stream; }
    private:
        SerializationStreamBase& m_Stream;
    };

#define IMPL_SERIALIZATION_WRAPPER(typeName, functionName)                                                         \
    template<> 																								       \
    inline void SerializationStream::Serialize<typeName>(std::string_view key, SerializationValue<typeName> value) \
    {                                                                                                              \
        m_Stream.PropertyKey(key);                                                                                 \
        m_Stream.functionName(value);                                                                              \
    }

    IMPL_SERIALIZATION_WRAPPER(int32_t, SerializeInt32);
    IMPL_SERIALIZATION_WRAPPER(uint32_t, SerializeUInt32);
    IMPL_SERIALIZATION_WRAPPER(float, SerializeFloat);

    template<>
    inline void SerializationStream::Serialize<glm::vec2>(std::string_view key, SerializationValue<glm::vec2> value)
    {
        m_Stream.PropertyKey(key);
        m_Stream.SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0])), 2);
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec3>(std::string_view key, SerializationValue<glm::vec3> value)
    {
        m_Stream.PropertyKey(key);
        m_Stream.SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0])), 3);
    }
}