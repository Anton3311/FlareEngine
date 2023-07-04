#include "Component.h"

namespace Flare
{
	bool operator==(const ComponentSet& setA, const ComponentSet& setB)
	{
		if (setA.GetCount() != setB.GetCount())
			return false;

		for (size_t i = 0; i < setA.GetCount(); i++)
		{
			if (setA[i] != setB[i])
				return false;
		}

		return true;
	}

	bool operator!=(const ComponentSet& setA, const ComponentSet& setB)
	{
		if (setA.GetCount() != setB.GetCount())
			return true;

		for (size_t i = 0; i < setA.GetCount(); i++)
		{
			if (setA[i] == setB[i])
				return true;
		}

		return false;
	}
}