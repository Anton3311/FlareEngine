#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Serialization/Serialization.h"

#define FLARE_SERIALIZABLE                                              \
    static Flare::SerializableObjectDescriptor _SerializationDescriptor;

#define FLARE_SERIALIZABLE_IMPL(typeName)                                              \
    Flare::SerializableObjectDescriptor typeName::_SerializationDescriptor(            \
        typeid(typeName).name(), sizeof(typeName),                                     \
        [](void* object, Flare::SerializationStream& stream) {                         \
            Flare::TypeSerializer<typeName>().OnSerialize(*(typeName*)object, stream); \
        });
