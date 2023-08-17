#include "ComponentInitializer.h"

namespace Flare
{
    ComponentInitializer::ComponentInitializer(const TypeInitializer& type)
        : m_Id(ComponentId()), Type(type)
    {
        GetInitializers().push_back(this);
    }

    ComponentInitializer::~ComponentInitializer()
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

    std::vector<ComponentInitializer*>& ComponentInitializer::GetInitializers()
    {
        static std::vector<ComponentInitializer*> s_Initializers;
        return s_Initializers;
    }
}
