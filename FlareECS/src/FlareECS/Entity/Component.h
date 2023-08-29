#pragma once

#include "Flare/Core/Assert.h"
#include "Flare/Core/Core.h"

#include <string>
#include <vector>
#include <xhash>
#include <functional>

namespace Flare
{
	struct ComponentId
	{
	public:
		constexpr ComponentId()
			: m_Index(UINT32_MAX), m_Generation(UINT16_MAX) {}
		constexpr ComponentId(uint32_t index, uint16_t generation)
			: m_Index(index), m_Generation(generation) {}

		constexpr ComponentId Masked() const
		{
			return ComponentId(m_Index & INDEX_MASK, m_Generation);
		}

		constexpr uint32_t GetIndex() const { return m_Index; }
		constexpr uint16_t GetGeneration() const { return m_Generation; }

		constexpr bool CompareMasked(ComponentId other) const
		{
			return (m_Index & INDEX_MASK) == (other.m_Index & INDEX_MASK) && m_Generation == other.m_Generation;
		}

		constexpr bool operator<(ComponentId other) const
		{
			return (m_Index & INDEX_MASK) < (other.m_Index & INDEX_MASK);
		}

		constexpr bool operator>(ComponentId other) const
		{
			return (m_Index & INDEX_MASK) > (other.m_Index & INDEX_MASK);
		}

		constexpr bool operator==(ComponentId other) const
		{
			return m_Index == other.m_Index && m_Generation == other.m_Generation;
		}

		constexpr bool operator!=(ComponentId other) const
		{
			return m_Index != other.m_Index || m_Generation != other.m_Generation;
		}

		static constexpr uint32_t INDEX_MASK = 0xffffffff >> 4;
	private:
		uint32_t m_Index;
		uint16_t m_Generation;

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
			Initializer(other.Initializer),
			Deleter(other.Deleter)
		{
		}

		ComponentInfo(ComponentInfo&& other) noexcept
			: Id(other.Id),
			RegistryIndex(other.RegistryIndex),
			Name(std::move(other.Name)),
			Size(other.Size),
			Initializer(other.Initializer),
			Deleter(std::move(other.Deleter))
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

		std::function<void(void*)> Deleter;
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

template<>
struct std::hash<Flare::ComponentId>
{
	size_t operator()(Flare::ComponentId id) const
	{
		return std::hash<uint32_t>()(id.m_Index & Flare::ComponentId::INDEX_MASK);
	}
};
