#pragma once

#include "FlareECS/Archetype.h"

#include <stdint.h>
#include <xhash>

namespace Flare
{
	struct Entity
	{
	public:
		Entity()
			: m_Packed(SIZE_MAX) {}
		Entity(uint32_t id)
			: m_Packed(0), m_Id(id), m_Generation(0) {}
		Entity(uint32_t id, uint16_t generation)
			: m_Packed(0), m_Id(id), m_Generation(generation) {}
	private:
		union
		{
			uint64_t m_Packed;
			struct
			{
				uint32_t m_Id;
				uint16_t m_Generation;
			};
		};

		friend struct std::hash<Entity>;
		friend bool operator==(const Entity& a, const Entity& b);
		friend bool operator!=(const Entity& a, const Entity& b);
	};

	bool operator==(const Entity& a, const Entity& b);
	bool operator!=(const Entity& a, const Entity& b);

	struct EntityRecord
	{
		size_t RegistryIndex;
		ArchetypeId Archetype;
		size_t BufferIndex;
	};
}

template<>
struct std::hash<Flare::Entity>
{
	size_t operator()(Flare::Entity entity) const
	{
		std::hash<uint64_t> hashFunction;
		return hashFunction(entity.m_Packed);
	}
};