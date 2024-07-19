#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Assert.h"

#include "FlareCore/Serialization/Metadata.h"

#include <glm/glm.hpp>

#include <vector>
#include <string_view>

namespace Flare
{
    class FLARECORE_API TypeInitializer
    {
    public:
        using DefaultConstructorFunction = void(*)(void*);
        using DestructorFunction = void(*)(void*);

        using CopyConstructorFunction = void(*)(void* instance, const void* copyFrom);
        using MoveConstructorFunction = void(*)(void* instance, void* moveFrom);

        TypeInitializer(std::string_view typeName, size_t size, 
            const SerializableObjectDescriptor& serializationDescriptor,
            DestructorFunction destructor, 
            DefaultConstructorFunction constructor,
            MoveConstructorFunction moveConstructor,
            CopyConstructorFunction copyConstructor);
        ~TypeInitializer();

        static std::vector<TypeInitializer*>& GetInitializers();
    public:
        const std::string_view TypeName;
        const DestructorFunction Destructor;
        const DefaultConstructorFunction DefaultConstructor;
        const CopyConstructorFunction CopyConstructor;
        const MoveConstructorFunction MoveConstructor;
        const size_t Size;
        const SerializableObjectDescriptor& SerializationDescriptor;
    };
}

#define FLARE_TYPE static Flare::TypeInitializer _Type; \
    FLARE_SERIALIZABLE

#define FLARE_IMPL_TYPE(typeName) Flare::TypeInitializer typeName::_Type =                            \
    Flare::TypeInitializer(typeid(typeName).name(), sizeof(typeName),                                 \
    FLARE_SERIALIZATION_DESCRIPTOR_OF(typeName),                                                      \
    [](void* instance) { ((typeName*)instance)->~typeName(); },                                       \
    [](void* instance) { new(instance) typeName;},                                                    \
    [](void* instance, void* moveFrom) { (*(typeName*)instance) = std::move(*(typeName*)moveFrom); }, \
    [](void* instance, const void* copyFrom) { (*(typeName*)instance) = *(typeName*)copyFrom; });     \
    FLARE_SERIALIZABLE_IMPL(typeName)