#pragma once

#include "FlareCore/Assert.h"

#include <string>
#include <vector>
#include <optional>

namespace Flare
{
#define FLARE_SERIALIZATION_DESCRIPTOR_OF(typeName) (typeName::_SerializationDescriptor)

    class SerializationStream;
    class SerializableObjectDescriptor
    {
    public:
        using SerializationCallback = void(*)(void* object, SerializationStream& stream);

        SerializableObjectDescriptor() = default;
        SerializableObjectDescriptor(std::string_view name, size_t size)
            : Name(name), Size(size), Callback(nullptr) {}

        SerializableObjectDescriptor(std::string_view name,
            size_t size,
            SerializationCallback callback)
            : Name(name),
            Size(size),
            Callback(callback) {}
    public:
        std::string Name;
        size_t Size;
        SerializationCallback Callback;
    };
}