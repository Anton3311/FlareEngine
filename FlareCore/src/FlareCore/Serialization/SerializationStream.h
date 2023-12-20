#pragma once

#include "FlareCore/Collections/Span.h"
#include "FlareCore/Serialization/TypeSerializer.h"

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

        SerializationValue(T* values, size_t size)
            : Values(Span(values, size)), IsArray(true) {}

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

        virtual void SerializeString(SerializationValue<std::string> value) = 0;

        virtual void BeginArray() = 0;
        virtual void EndArray() = 0;

        virtual void BeginObject(const SerializableObjectDescriptor* descriptor) = 0;
        virtual void EndObject() = 0;
    };

    class SerializationStream
    {
    public:
        SerializationStream(SerializationStreamBase& stream)
            : m_Stream(stream) {}

        template<typename T>
        inline void Serialize(SerializationValue<T> value)
        {
            TypeSerializer<T> serializer;
            const SerializableObjectDescriptor* descriptor = SerializationDescriptorOf<T>().Descriptor();
            if (value.IsArray)
            {
                m_Stream.BeginArray();
                for (size_t i = 0; i < value.Values.GetSize(); i++)
                {
                    m_Stream.BeginObject(descriptor);
                    serializer.OnSerialize(value.Values[i], *this);
                    m_Stream.EndObject();
                }
                m_Stream.EndArray();
            }
            else
            {
                if (descriptor)
                    m_Stream.BeginObject(descriptor);

                serializer.OnSerialize(value.Values[0], *this);

                if (descriptor)
                    m_Stream.EndObject();
            }
        }

        template<typename T>
        inline void Serialize(std::string_view key, SerializationValue<T> value)
        {
            m_Stream.PropertyKey(key);
            Serialize(value);
        }

        inline SerializationStreamBase& GetInternalStream() { return m_Stream; }
    private:
        SerializationStreamBase& m_Stream;
    };

#define IMPL_SERIALIZATION_WRAPPER(typeName, functionName)                                    \
    template<> 																				  \
    inline void SerializationStream::Serialize<typeName>(SerializationValue<typeName> value)  \
    {                                                                                         \
        if (value.IsArray) m_Stream.BeginArray();                                             \
        m_Stream.functionName(value);                                                         \
        if (value.IsArray) m_Stream.EndArray();                                               \
    }

    IMPL_SERIALIZATION_WRAPPER(int32_t, SerializeInt32);
    IMPL_SERIALIZATION_WRAPPER(uint32_t, SerializeUInt32);
    IMPL_SERIALIZATION_WRAPPER(float, SerializeFloat);

    template<>
    inline void SerializationStream::Serialize<std::string>(SerializationValue<std::string> value)
    {
        if (value.IsArray)
        {
            m_Stream.BeginArray();
            m_Stream.SerializeString(value);
            m_Stream.EndArray();
        }
        else
            m_Stream.SerializeString(value);
    }

    template<>
    inline void SerializationStream::Serialize<glm::ivec2>(SerializationValue<glm::ivec2> value)
    {
        if (value.IsArray)
        {
            m_Stream.BeginArray();
            int32_t* vectors = glm::value_ptr(value.Values[0]);
            m_Stream.SerializeIntVector(SerializationValue(vectors, value.Values.GetSize() * 2), 2);
            m_Stream.EndArray();
        }
        else
        {
            m_Stream.SerializeIntVector(SerializationValue(*glm::value_ptr(value.Values[0])), 2);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::ivec3>(SerializationValue<glm::ivec3> value)
    {
        if (value.IsArray)
        {
            m_Stream.BeginArray();
            int32_t* vectors = glm::value_ptr(value.Values[0]);
            m_Stream.SerializeIntVector(SerializationValue(vectors, value.Values.GetSize() * 3), 3);
            m_Stream.EndArray();
        }
        else
        {
            m_Stream.SerializeIntVector(SerializationValue(*glm::value_ptr(value.Values[0])), 3);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec2>(SerializationValue<glm::vec2> value)
    {
        if (value.IsArray)
        {
            m_Stream.BeginArray();
            float* vectors = glm::value_ptr(value.Values[0]);
            m_Stream.SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 2), 2);
            m_Stream.EndArray();
        }
        else
        {
            m_Stream.SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0])), 2);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec3>(SerializationValue<glm::vec3> value)
    {
        if (value.IsArray)
        {
            m_Stream.BeginArray();
            float* vectors = glm::value_ptr(value.Values[0]);
            m_Stream.SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 3), 3);
            m_Stream.EndArray();
        }
        else
        {
            m_Stream.SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0])), 3);
        }
    }

    template<typename T>
    struct SerializationDescriptorOf<std::vector<T>>
    {
        const SerializableObjectDescriptor* Descriptor()
        {
            return nullptr;
        }
    };

    template<typename T>
    struct TypeSerializer<std::vector<T>>
    {
        void OnSerialize(std::vector<T>& vector, SerializationStream& stream)
        {
            stream.Serialize(SerializationValue(vector.data(), vector.size()));
        }
    };
}