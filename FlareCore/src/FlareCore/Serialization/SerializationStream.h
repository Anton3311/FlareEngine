#pragma once

#include "FlareCore/UUID.h"
#include "FlareCore/Collections/Span.h"
#include "FlareCore/Serialization/TypeSerializer.h"

#include <stdint.h>
#include <string_view>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Flare
{
    enum class SerializationValueFlags
    {
        None = 0,
        Color = 1,
        HDRColor = 2,
    };

    FLARE_IMPL_ENUM_BITFIELD(SerializationValueFlags);

    template<typename T>
    struct SerializationValue
    {
        SerializationValue(T& value, SerializationValueFlags flags = SerializationValueFlags::None)
            : Values(Span(value)), IsArray(false), Flags(flags) {}

        SerializationValue(T* values, size_t size, SerializationValueFlags flags = SerializationValueFlags::None)
            : Values(Span(values, size)), IsArray(true), Flags(flags) {}

        SerializationValue(const Span<T>& values, SerializationValueFlags flags = SerializationValueFlags::None)
            : Values(values), IsArray(true), Flags(flags) {}

        Span<T> Values;
        const bool IsArray;
        const SerializationValueFlags Flags;
    };

    enum class SerializableIntType
    {
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
        Int64,
        UInt64,
    };

    inline size_t SizeOfSerializableIntType(SerializableIntType type)
    {
#define SIZEOF_INT(intType, enumType)       \
        case SerializableIntType::enumType: \
            return sizeof(intType)

        switch (type)
        {
            SIZEOF_INT(int8_t, Int8);
            SIZEOF_INT(uint8_t, UInt8);
            SIZEOF_INT(int16_t, Int16);
            SIZEOF_INT(uint16_t, UInt16);
            SIZEOF_INT(int32_t, Int32);
            SIZEOF_INT(uint32_t, UInt32);
            SIZEOF_INT(int64_t, Int64);
            SIZEOF_INT(uint64_t, UInt64);
        }

#undef SIZEOF_INT

        FLARE_CORE_ASSERT(false);
        return 0;
    }

    template<typename T>
    constexpr bool IsReferenceCounted = false;

    template<typename T>
    constexpr bool IsReferenceCounted<Ref<T>> = true;

    template<typename T>
    struct ReferenceCountUnderlyingType
    {
        using Type = void;
    };

    template<typename T>
    struct ReferenceCountUnderlyingType<Ref<T>>
    {
        using Type = T;
    };

    template<typename T>
    struct ReferenceCountValuePointer
    {
        inline void* Get(T& ref)
        {
            return nullptr;
        }
    };

    template<typename T>
    struct ReferenceCountValuePointer<Ref<T>>
    {
        inline void* Get(Ref<T>& ref)
        {
            return ref.get();
        }
    };



    class SerializableObjectDescriptor;
    class SerializationStream
    {
    public:
        enum class DynamicArrayAction
        {
            None,
            Resize,
            Append,
        };

        virtual void PropertyKey(std::string_view key) = 0;
        virtual DynamicArrayAction SerializeDynamicArraySize(size_t& size) = 0;

        virtual void SerializeInt(SerializationValue<uint8_t> intValues, SerializableIntType type) = 0;
        virtual void SerializeBool(SerializationValue<bool> value) = 0;
        virtual void SerializeFloat(SerializationValue<float> value) = 0;

        virtual void SerializeUUID(SerializationValue<UUID> uuids) = 0;

        virtual void SerializeFloatVector(SerializationValue<float> value, uint32_t componentsCount) = 0;
        virtual void SerializeIntVector(SerializationValue<int32_t> value, uint32_t componentsCount) = 0;

        virtual void SerializeString(SerializationValue<std::string> value) = 0;
        virtual void SerializeObject(const SerializableObjectDescriptor& descriptor, void* objectData, bool isArray, size_t arraySize) {}

        virtual void SerializeReference(const SerializableObjectDescriptor& valueDescriptor,
            void* referenceData,
            void* valueData) = 0;

        template<typename T>
        inline void Serialize(SerializationValue<T> value)
        {
            if constexpr (IsReferenceCounted<T>)
            {
                SerializeReference(
                    *SerializationDescriptorOf<ReferenceCountUnderlyingType<T>::Type>().Descriptor(),
                    &value.Values[0],
                    ReferenceCountValuePointer<T>().Get(value.Values[0]));
                return;
            }

            const SerializableObjectDescriptor* descriptor = SerializationDescriptorOf<T>().Descriptor();

            FLARE_CORE_ASSERT(descriptor);
            if (value.IsArray)
            {
				SerializeObject(*descriptor, value.Values.GetData(), true, value.Values.GetSize());
            }
            else
            {
                SerializeObject(*descriptor, &value.Values[0], false, 0);
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
        functionName(value);                                                                  \
    }

#define IMPL_INT_SERIALIZATION_WRAPPER(typeName, intType)                                     \
    template<> 																				  \
    inline void SerializationStream::Serialize<typeName>(SerializationValue<typeName> value)  \
    {                                                                                         \
        if (value.IsArray)                                                                    \
            SerializeInt(SerializationValue(                                                  \
                (uint8_t*)(value.Values.GetData()),                                           \
                value.Values.GetSize() * sizeof(typeName), value.Flags), intType);            \
        else                                                                                  \
            SerializeInt(SerializationValue(                                                  \
                *(uint8_t*)(value.Values.GetData()),                                          \
                value.Flags), intType);                                                       \
    }

    IMPL_SERIALIZATION_WRAPPER(float, SerializeFloat);
    IMPL_SERIALIZATION_WRAPPER(bool, SerializeBool);
    IMPL_SERIALIZATION_WRAPPER(UUID, SerializeUUID);

    IMPL_INT_SERIALIZATION_WRAPPER(int8_t, SerializableIntType::Int8);
    IMPL_INT_SERIALIZATION_WRAPPER(uint8_t, SerializableIntType::UInt8);
    IMPL_INT_SERIALIZATION_WRAPPER(int16_t, SerializableIntType::Int16);
    IMPL_INT_SERIALIZATION_WRAPPER(uint16_t, SerializableIntType::UInt16);
    IMPL_INT_SERIALIZATION_WRAPPER(int32_t, SerializableIntType::Int32);
    IMPL_INT_SERIALIZATION_WRAPPER(uint32_t, SerializableIntType::UInt32);
    IMPL_INT_SERIALIZATION_WRAPPER(int64_t, SerializableIntType::Int64);
    IMPL_INT_SERIALIZATION_WRAPPER(uint64_t, SerializableIntType::UInt64);

    template<>
    inline void SerializationStream::Serialize<std::string>(SerializationValue<std::string> value)
    {
        SerializeString(value);
    }

    template<>
    inline void SerializationStream::Serialize<glm::ivec2>(SerializationValue<glm::ivec2> value)
    {
        if (value.IsArray)
        {
            int32_t* vectors = glm::value_ptr(value.Values[0]);
            SerializeIntVector(SerializationValue(vectors, value.Values.GetSize() * 2, value.Flags), 2);
        }
        else
        {
            SerializeIntVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 2);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::ivec3>(SerializationValue<glm::ivec3> value)
    {
        if (value.IsArray)
        {
            int32_t* vectors = glm::value_ptr(value.Values[0]);
            SerializeIntVector(SerializationValue(vectors, value.Values.GetSize() * 3, value.Flags), 3);
        }
        else
        {
            SerializeIntVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 3);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::ivec4>(SerializationValue<glm::ivec4> value)
    {
        if (value.IsArray)
        {
            int32_t* vectors = glm::value_ptr(value.Values[0]);
            SerializeIntVector(SerializationValue(vectors, value.Values.GetSize() * 4, value.Flags), 4);
        }
        else
        {
            SerializeIntVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 4);
        }
    }



    template<>
    inline void SerializationStream::Serialize<glm::vec2>(SerializationValue<glm::vec2> value)
    {
        if (value.IsArray)
        {
            float* vectors = glm::value_ptr(value.Values[0]);
            SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 2, value.Flags), 2);
        }
        else
        {
            SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 2);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec3>(SerializationValue<glm::vec3> value)
    {
        if (value.IsArray)
        {
            float* vectors = glm::value_ptr(value.Values[0]);
            SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 3, value.Flags), 3);
        }
        else
        {
            SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 3);
        }
    }

    template<>
    inline void SerializationStream::Serialize<glm::vec4>(SerializationValue<glm::vec4> value)
    {
        if (value.IsArray)
        {
            float* vectors = glm::value_ptr(value.Values[0]);
            SerializeFloatVector(SerializationValue(vectors, value.Values.GetSize() * 4, value.Flags), 4);
        }
        else
        {
            SerializeFloatVector(SerializationValue(*glm::value_ptr(value.Values[0]), value.Flags), 4);
        }
    }

    template<typename T>
    struct TypeSerializer<std::vector<T>>
    {
        static void OnSerialize(std::vector<T>& vector, SerializationStream& stream)
        {
            // TODO: should probably assert that T is default constructable

            size_t size = vector.size();
            switch (stream.SerializeDynamicArraySize(size))
            {
            case SerializationStream::DynamicArrayAction::None:
                break;
            case SerializationStream::DynamicArrayAction::Append:
                vector.emplace_back();
                break;
            case SerializationStream::DynamicArrayAction::Resize:
                vector.resize(size);
                break;
            }

            stream.Serialize(SerializationValue(vector.data(), vector.size()));
        }
    };

    template<typename T>
    struct SerializationDescriptorOf<std::vector<T>>
    {
        static const SerializableObjectDescriptor* Descriptor()
        {
            static SerializableObjectDescriptor s_Descriptor(
                typeid(std::vector<T>).name(),
                sizeof(std::vector<T>),
                [](void* vector, SerializationStream& stream)
                {
                    TypeSerializer<std::vector<T>>::OnSerialize(*(std::vector<T>*)vector, stream);
                });

            return &s_Descriptor;
        }
    };



    template<typename T>
    struct TypeSerializer<Ref<T>>
    {
        static void OnSerialize(Ref<T>& ref, SerializationStream& stream)
        {
            const auto* descriptor = SerializationDescriptorOf<T>().Descriptor();
            FLARE_CORE_ASSERT(descriptor);

            stream.SerializeReference(*descriptor, &ref, ref.get());
        }
    };

    template<typename T>
    struct SerializationDescriptorOf<Ref<T>>
    {
        static const SerializableObjectDescriptor* Descriptor()
        {
            static SerializableObjectDescriptor s_Descriptor(
                typeid(Ref<T>).name(),
                sizeof(Ref<T>),
                [](void* vector, SerializationStream& stream)
                {
                    TypeSerializer<Ref<T>>::OnSerialize(*(Ref<T>*)vector, stream);
                });

            return &s_Descriptor;
        }
    };
}