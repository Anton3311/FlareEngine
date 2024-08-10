#pragma once

#include "FlareCore/Serialization/TypeInitializer.h"
#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore/Serialization/Metadata.h"

#include <stdint.h>
#include <xhash>

namespace Flare
{
	struct FLAREECS_API Entity
	{
	public:
		FLARE_TYPE;
		FLARE_SERIALIZABLE;

		constexpr Entity()
			: m_Index(UINT32_MAX), m_Generation(UINT16_MAX) {}
		constexpr Entity(uint32_t id)
			: m_Index(id), m_Generation(0) {}
		constexpr Entity(uint32_t id, uint16_t generation)
			: m_Index(id), m_Generation(generation) {}

		constexpr uint32_t GetIndex() const { return m_Index; }
		constexpr uint16_t GetGeneration() const { return m_Generation; }

		constexpr bool operator==(Entity other) const
		{
			return m_Index == other.m_Index && m_Generation == other.m_Generation;
		}

		constexpr bool operator!=(Entity other) const
		{
			return m_Index != other.m_Index || m_Generation != other.m_Generation;
		}
	private:
		uint32_t m_Index;
		uint16_t m_Generation;

		friend struct std::hash<Entity>;
	};
}

template<>
struct std::hash<Flare::Entity>
{
	size_t operator()(Flare::Entity entity) const
	{
		std::hash<uint64_t> hashFunction;
		return hashFunction(entity.m_Index);
	}
};

