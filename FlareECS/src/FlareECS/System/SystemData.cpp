#include "SystemData.h"

namespace Flare
{
	void SystemData::AddDependecy(SystemId system)
	{
		m_Dependecies.insert(m_Dependecies.begin() + m_DependecyCount, system);
		m_DependecyCount++;
	}

	void SystemData::AddDependentSystem(SystemId system)
	{
		m_Dependecies.push_back(system);
	}
}
