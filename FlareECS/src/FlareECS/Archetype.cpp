#include "Archetype.h"

namespace Flare
{
	std::optional<size_t> Archetype::FindComponent(ComponentId component)
	{
		size_t left = 0;
		size_t right = Components.size();

		while (right - left > 1)
		{
			size_t mid = (right + left) / 2;
			if (Components[mid] == component)
			{
				return { mid };
			}

			if (Components[mid] > component)
				right = mid;
			else
				left = mid;
		}

		if (Components[left] == component)
			return { left };

		return {};
	}
}