#include "TypeInitializer.h"

#include "FlareCore/Assert.h"

namespace Flare
{
    TypeInitializer::TypeInitializer(std::string_view typeName, size_t size, size_t alignment,
        const TypeConstructorFunctions& constructorFunctions,
        TypeFlags flags)
        : TypeName(typeName), Size(size), Alignment(alignment), Functions(constructorFunctions), Flags(flags)
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