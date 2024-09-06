#pragma once

#include <stdint.h>

namespace Flare
{
	class QueryChunkEntity
	{
	public:
		constexpr QueryChunkEntity(uint8_t* entityData)
			: m_EntityData(entityData) {}
	public:
		uint8_t* GetEntityData() const { return m_EntityData; }
	private:
		uint8_t* m_EntityData;
	};
}
