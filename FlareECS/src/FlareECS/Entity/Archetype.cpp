#include "Archetype.h"

namespace Flare
{
    std::optional<size_t> ArchetypeRecord::TryGetComponentIndex(ComponentId component) const
    {
        size_t left = 0;
        size_t right = Components.size();

        while (right - left > 1)
        {
            size_t mid = (left + right) / 2;
            if (Components[mid] == component)
                return mid;
            else if (Components[mid] < component)
                left = mid;
            else
                right = mid;
        }

        if (Components[left] == component)
            return left;
        return {};
    }
}