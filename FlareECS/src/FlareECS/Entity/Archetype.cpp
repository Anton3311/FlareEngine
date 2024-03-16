#include "Archetype.h"

namespace Flare
{
    std::optional<size_t> ArchetypeRecord::TryGetComponentIndex(ComponentId component) const
    {
        size_t left = 0;
        size_t right = Components.size();

        while (left <= right)
        {
            size_t mid = (right + left) / 2;
            if (Components[mid] == component)
                return mid;
            else if (Components[mid] < component)
                left = mid + 1;
            else
                right = mid - 1;
        }

        if (Components[left] == component)
            return left;
        return {};
    }
}