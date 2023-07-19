#pragma once

#include "FlareScriptingCore/Defines.h"

#include <string>
#include <string_view>
#include <vector>
#include <typeinfo>
#include <iostream>

namespace Flare::Internal
{
    class ScriptingType
    {
    public:
        using ConstructorFunction = void*(*)();
        using DestructorFunction = void(*)(void*);

        ScriptingType(std::string_view name, size_t size, ConstructorFunction constructor, DestructorFunction destructor, DestructorFunction deleter)
            : Name(name), Size(size), Constructor(constructor), Destructor(destructor), Deleter(deleter)
        {
            GetRegisteredTypes().push_back(this);
        }

        const std::string_view Name;
        const size_t Size;

        const ConstructorFunction Constructor;
        const DestructorFunction Destructor;
        const DestructorFunction Deleter;
    public:
        static std::vector<const ScriptingType*>& GetRegisteredTypes();
    };
}

#define FLARE_DEFINE_SCRIPTING_TYPE(type) public:   \
    static Flare::ScriptingType FlareScriptingType; \
    static void* CreateInstance();                  \
    static void DeleteInstance(void* instance);     \
    static void DestroyInstance(void* instance);

#define FLARE_IMPL_SCRIPTING_TYPE(type)                                         \
    void* type::CreateInstance() { return new type(); }                         \
    void type::DeleteInstance(void* instance) { delete (type*)instance; }       \
    void type::DestroyInstance(void* instance) { ((type*)instance)->~type(); }  \
    Flare::ScriptingType type::FlareScriptingType = Flare::ScriptingType(		\
        typeid(type).name(), 													\
        sizeof(type), 															\
        type::CreateInstance,													\
        type::DestroyInstance,                                                  \
        type::DeleteInstance);