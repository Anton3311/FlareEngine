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
    class SerializationStream
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

        template<typename T>
        inline void Serialize(SerializationValue<T> value)
        {
            TypeSerializer<T> serializer;
            const SerializableObjectDescriptor* descriptor = SerializationDescriptorOf<T>().Descriptor();
            if (value.IsArray)
            {
                BeginArray();
                if (descriptor != nullptr)
                {
                    for (size_t i = 0; i < value.Values.GetSize(); i++)
                    {
                        BeginObject(descriptor);
                        serializer.OnSerialize(value.Values[i], *this);
                        EndObject();
                    }
                }
                else
                {
                    for (size_t i = 0; i < value.Values.GetSize(); i++)
                    {
                        serializer.OnSerialize(value.Values[i], *this);
                    }
                }
                EndArray();
            }
            else
            {
                if (descriptor)
                    BeginObject(descriptor);

                serializer.OnSerialize(value.Values[0], *this);

                if (descriptor)
                    EndObject();
            }
        }

        template<typename T>
        inline void Serialize(std::string_view key, SerializationValue<T> value)
        {
            PropertyKey(key);
            Serialize(value);
        }
    };

#define IMPL_SERIALIZATION_WRAPPER(typeName, functionName)                                    \
    template<> 																				  \
    inline void SerializationStream::Serialize<typeName>(SerializationValue<typeName> value)  \
    {                                                                                         \
        if (value.IsArray) BeginArray();                                                      \
        functionName(value);                                                                  \
        if (value.IsArray) EndArray();                                                        \
    }

    IMPL_SERIALIZATION_WRAPPER(int32_t, SerializeInt32);
    IMPL_SERIALIZATION_WRAPPER(uint32_t, SerializeUInt32);
    IMPL_SERIALIZATION_WRAPPER(float, SerializeFloat);

    template<>
    inline void SerializationStream::Serialize<std::string>(SerializationValue<std::string> value)
    {
        if (value.IsArray)
        {
            BeginArray();
            SerializeString(value);
            EndArray();
        }
        else
            SerializeString(value);
    }

    template<>
    inline void SerializationStream::Serialize<glm::ivec2>(SerializationValue<glm::ivec2> value)
    {
        if (value.IsArray)
        {
            BeginArray();
            int32_t* vectors = glm::value_ptr(value.Values[0]);
            SerializeIntVector(SerializationValue(vectors, value.Values.GetSize() * 2), 2);
            EndArray();
        }
        else
        {
            SerializeIntVector(SerializationValue(*glm::value_ptr(value.Values[0])), 2);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::ivec3>(SerializationValue<glm::ivec3> value)
    {
        if (value.IsArray)
        {
            BeginArray();
            int32_t* vectors = glm::value_ptr(value.Values[0]);
            SerializeIntVector(SerializationValue(vectors, value.Values.GetSize() * 3), 3);
            EndArray();
        }
        else
        {
            SerializeIntVector(SerializationValue(*glm::value_ptr(value.Values[0])), 3);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec2>(SerializationValue<glm::vec2> value)
    {
        if (value.IsArray)
        {
            BeginArray();
            float* vectors = glm::value_ptr(value.Values[0]);
            SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 2), 2);
            EndArray();
        }
        else
        {
            SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0])), 2);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec3>(SerializationValue<glm::vec3> value)
    {
        if (value.IsArray)
        {
            BeginArray();
            float* vectors = glm::value_ptr(value.Values[0]);
            SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 3), 3);
            EndArray();
        }
        else
        {
            SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0])), 3);
        }
    }

    template<typename T>
    struct SerializationDescriptorOf<std::vector<T>>
    {
        static const SerializableObjectDescriptor* Descriptor()
        {
            return nullptr;
        }
    };

    template<typename T>
    struct TypeSerializer<std::vector<T>>
    {
        static void OnSerialize(std::vector<T>& vector, SerializationStream& stream)
        {
            stream.Serialize(SerializationValue(vector.data(), vector.size()));
        }
    };
}