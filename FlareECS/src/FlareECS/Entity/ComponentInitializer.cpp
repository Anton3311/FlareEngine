#include "ComponentInitializer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/Entity/Components.h"

namespace Flare
{
    ComponentInitializer::ComponentInitializer(const TypeInitializer& type, const SerializableObjectDescriptor& serializationDescriptor)
        : m_Id(ComponentId()), Type(type), SerializationDescriptor(serializationDescriptor)
    {
        GetInitializers().push_back(this);
    }

    ComponentInitializer::~ComponentInitializer()
    {
        FLARE_PROFILE_FUNCTION();

        auto& initializers = GetInitializers();
        for (size_t i = 0; i < initializers.size(); i++)
        {
            if (initializers[i] == this)
            {
                initializers.erase(initializers.begin() + i);
                break;
            }
        }

        if (IsRegistered())
        {
            m_CorrespondingRegistry->OnComponentUnregister(*this);
        }
    }

    std::vector<ComponentInitializer*>& ComponentInitializer::GetInitializers()
    {
        static std::vector<ComponentInitializer*> s_Initializers;
        return s_Initializers;
    }
}
