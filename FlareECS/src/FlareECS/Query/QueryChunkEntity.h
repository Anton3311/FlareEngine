#pragma once

#include <stdint.h>

namespace Flare
{
	class QueryChunkEntity
	{
	public:
		constexpr QueryChunkEntity(uint8_t* entityData, size_t entityIndex)
			: m_EntityData(entityData), m_EntityIndex(entityIndex) {}
	public:
		constexpr uint8_t* GetEntityData() const { return m_EntityData; }
		constexpr size_t GetEntityIndex() const { return m_EntityIndex; }
	private:
		uint8_t* m_EntityData = nullptr;
		size_t m_EntityIndex = 0;
	};
}
