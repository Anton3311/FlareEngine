#include "SystemInitializer.h"

#include "FlareCore/Log.h"

namespace Flare
{
	SystemInitializer::SystemInitializer(const char* name, CreateSystemFunction createSystem)
		: TypeName(name), CreateSystem(createSystem), m_Id(UINT32_MAX)
	{
		std::vector<SystemInitializer*>& initializers = GetInitializers();
		initializers.push_back(this);
	}

	SystemInitializer::~SystemInitializer()
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

	std::vector<SystemInitializer*>& Flare::SystemInitializer::GetInitializers()
	{
		static std::vector<SystemInitializer*> s_Initializers;
		return s_Initializers;
	}
}
