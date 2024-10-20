#pragma once

#include "FlareCore/Assert.h"
#include "FlareCore/Core.h"

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

		ComponentInfo(const ComponentInfo& other)
			: Id(other.Id),
			RegistryIndex(other.RegistryIndex),
			Name(other.Name),
			Size(other.Size),
			Initializer(other.Initializer)
		{
		}

		ComponentInfo(ComponentInfo&& other) noexcept
			: Id(other.Id),
			RegistryIndex(other.RegistryIndex),
			Name(std::move(other.Name)),
			Size(other.Size),
			Initializer(other.Initializer)
		{
			other.Id = ComponentId();
			other.RegistryIndex = UINT32_MAX;
			other.Size = 0;
			other.Initializer = nullptr;
		}

		ComponentId Id;
		uint32_t RegistryIndex;
		std::string Name;
		size_t Size;

		ComponentInitializer* Initializer;
	};

	class ComponentSet
	{
	public:
		ComponentSet(const std::vector<ComponentId>& ids)
			: m_Ids(ids.data()), m_Count(ids.size())
		{
			FLARE_CORE_ASSERT(m_Count, "Components count shouldn't been 0");
		}

		constexpr ComponentSet(const ComponentId* ids, size_t count)
			: m_Ids(ids), m_Count(count) {}

		constexpr const ComponentId* GetIds() { return m_Ids; }
		constexpr const ComponentId* GetIds() const { return m_Ids; }
		constexpr size_t GetCount() const { return m_Count; }

		constexpr ComponentId operator[](size_t index) const
		{
			FLARE_CORE_ASSERT(index < m_Count);
			return m_Ids[index];
		}

		constexpr const ComponentId* begin() const
		{
			return m_Ids;
		}

		constexpr const ComponentId* end() const
		{
			return m_Ids + m_Count;
		}
	private:
		const ComponentId* m_Ids;
		size_t m_Count;
	};

	FLAREECS_API bool operator==(const ComponentSet& setA, const ComponentSet& setB);
	FLAREECS_API bool operator!=(const ComponentSet& setA, const ComponentSet& setB);
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

template<>
struct std::hash<Flare::ComponentId>
{
	size_t operator()(Flare::ComponentId id) const
	{
		return std::hash<Flare::ComponentId::UnderlyingType>()(id.m_Value);
	}
};
