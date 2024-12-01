#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include <string>
#include <vector>
#include <xhash>
#include <functional>

namespace Flare
{
	struct ComponentId
	{
	public:
		using UnderlyingType = uint16_t;
		static constexpr UnderlyingType INDEX_BITS = 10;
		static constexpr UnderlyingType GENERATION_BITS = 6;
		static constexpr UnderlyingType INDEX_MASK = (1 << INDEX_BITS) - 1;
		static constexpr UnderlyingType GENERATION_MASK = (1 << GENERATION_BITS) - 1;

		constexpr ComponentId()
			: m_Value(std::numeric_limits<UnderlyingType>::max()) {}

		inline ComponentId(UnderlyingType index, UnderlyingType generation)
			: m_Value(index | generation << INDEX_BITS)
		{
			FLARE_CORE_VERIFY(index < INDEX_MASK && generation < GENERATION_MASK);
		}

		constexpr UnderlyingType GetIndex() const { return m_Value & INDEX_MASK; }
		constexpr UnderlyingType GetGeneration() const { return (m_Value & GENERATION_MASK) >> INDEX_BITS; }

		constexpr bool operator<(ComponentId other) const
		{
			return GetIndex() < other.GetIndex();
		}

		constexpr bool operator>(ComponentId other) const
		{
			return GetIndex() > other.GetIndex();
		}

		constexpr bool operator==(ComponentId other) const
		{
			return m_Value == other.m_Value;
		}

		constexpr bool operator!=(ComponentId other) const
		{
			return m_Value != other.m_Value;
		}

		constexpr UnderlyingType GetValue() const { return m_Value; }
	private:
		UnderlyingType m_Value;

		friend struct std::hash<ComponentId>;
	};

	class ComponentInitializer;
	struct ComponentInfo
	{
		ComponentInfo()
			: Id(ComponentId()),
			RegistryIndex(UINT32_MAX),
			Size(0), Initializer(nullptr) {}

		ComponentId Id;
		uint32_t RegistryIndex;
		std::string Name;
		size_t Size;

		ComponentInitializer* Initializer;
	};

	using ComponentSet = Span<const ComponentId>;
}

template<>
struct std::hash<Flare::ComponentSet>
{
	size_t operator()(const Flare::ComponentSet& set) const
	{
		size_t hash = 0;
		for (size_t i = 0; i < set.GetSize(); i++)
			Flare::CombineHashes<Flare::ComponentId>(hash, set[i]);

		return hash;
	}
};

template<>
struct std::hash<Flare::ComponentId>
{
	size_t operator()(Flare::ComponentId id) const
	{
		return std::hash<Flare::ComponentId::UnderlyingType>()(id.m_Value);
	}
};
