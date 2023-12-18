#pragma once

namespace Flare
{
    class SerializationStream;
    class SerializableObjectDescriptor;

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