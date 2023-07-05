#include "Entity.h"

namespace Flare
{
	bool operator==(const Entity& a, const Entity& b)
	{
		return a.m_Id == b.m_Id && a.m_Generation == b.m_Generation;
	}

	bool operator!=(const Entity& a, const Entity& b)
	{
		return a.m_Id != b.m_Id || a.m_Generation != b.m_Generation;
	}
}