#pragma once

namespace Flare
{
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
}