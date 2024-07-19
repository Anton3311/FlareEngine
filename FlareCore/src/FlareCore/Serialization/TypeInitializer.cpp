#include "TypeInitializer.h"

#include "FlareCore/Assert.h"

namespace Flare
{
    TypeInitializer::TypeInitializer(std::string_view typeName, size_t size, 
        const SerializableObjectDescriptor& serializationDescriptor,
        DestructorFunction destructor,
        DefaultConstructorFunction constructor,
        MoveConstructorFunction moveConstructor,
        CopyConstructorFunction copyConstructor)
        : TypeName(typeName), Size(size), 
          SerializationDescriptor(serializationDescriptor),
          Destructor(destructor),
          DefaultConstructor(constructor),
          MoveConstructor(moveConstructor),
          CopyConstructor(copyConstructor)
    {
        GetInitializers().push_back(this);
    }

    TypeInitializer::~TypeInitializer()
    {
        auto& initializers = GetInitializers();
        for (size_t i = 0; i < initializers.size(); i++)
        {
            if (initializers[i] == this)
            {
                initializers.erase(initializers.begin() + i);
                break;
            }
        }
    }

    std::vector<TypeInitializer*>& TypeInitializer::GetInitializers()
    {
        static std::vector<TypeInitializer*> s_Initializers;
        return s_Initializers;
    }
}