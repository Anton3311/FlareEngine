#pragma once

#include "Flare/Core/Assert.h"
#include "Flare/Core/Core.h"

#include <string>
#include <vector>
#include <xhash>

namespace Flare
{
	using ComponentId = size_t;
	struct ComponentInfo
	{
		ComponentId Id;
		std::string Name;
		size_t Size;
	};

	class ComponentSet
	{
	public:
		ComponentSet(std::vector<ComponentId>& ids)
			: m_Ids(ids.data()), m_Count(ids.size())
		{
			FLARE_CORE_ASSERT(m_Count, "Components count shouldn't been 0");
		}

		constexpr ComponentSet(ComponentId* ids, size_t count)
			: m_Ids(ids), m_Count(count) {}

		constexpr ComponentId* GetIds() { return m_Ids; }
		constexpr const ComponentId* GetIds() const { return m_Ids; }
		constexpr size_t GetCount() const { return m_Count; }

		constexpr ComponentId operator[](size_t index) const
		{
			FLARE_CORE_ASSERT(index < m_Count);
			return m_Ids[index];
		}
	private:
		ComponentId* m_Ids;
		size_t m_Count;
	};

	bool operator==(const ComponentSet& setA, const ComponentSet& setB);
	bool operator!=(const ComponentSet& setA, const ComponentSet& setB);
}

template<>
struct std::hash<Flare::ComponentSet>
{
	size_t operator()(const Flare::ComponentSet& set) const
	{
		size_t hash = 0;
		for (size_t i = 0; i < set.GetCount(); i++)
			Flare::CombineHashes<Flare::ComponentId>(hash, set[i]);

		return hash;
	}
};