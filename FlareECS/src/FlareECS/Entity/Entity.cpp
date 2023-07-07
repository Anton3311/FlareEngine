#include "Entity.h"

namespace Flare
{
	bool operator==(const Entity& a, const Entity& b)
	{
		return a.m_Index == b.m_Index && a.m_Generation == b.m_Generation;
	}

	bool operator!=(const Entity& a, const Entity& b)
	{
		return a.m_Index != b.m_Index || a.m_Generation != b.m_Generation;
	}
}