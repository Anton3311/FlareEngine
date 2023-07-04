#pragma once

#include "FlareECS/EntityStorage/EntityStorage.h"

namespace Flare
{
	class EntityViewElement
	{
	public:
		EntityViewElement(uint8_t* entityData)
			: m_EntityData(entityData) {}
	public:
		uint8_t* GetEntityData() const { return m_EntityData; }
	private:
		uint8_t* m_EntityData;
	};

	class EntityViewIterator
	{
	public:
		EntityViewIterator(EntityStorage& storage, size_t index)
			: m_Storage(storage), m_EntityIndex(index) {}

		inline EntityViewIterator operator++()
		{
			m_EntityIndex++;
			return *this;
		}

		inline bool operator==(const EntityViewIterator& other)
		{
			return &m_Storage == &other.m_Storage && m_EntityIndex == other.m_EntityIndex;
		}

		inline bool operator!=(const EntityViewIterator& other)
		{
			return &m_Storage != &other.m_Storage || m_EntityIndex != other.m_EntityIndex;
		}

		inline EntityViewElement operator*() const
		{
			return EntityViewElement(m_Storage.GetEntityData(m_EntityIndex));
		}

		inline uint8_t* GetEntityData() const { return m_Storage.GetEntityData(m_EntityIndex); }
		inline EntityStorage& GetStorage() { return m_Storage; }
		inline size_t GetEntityIndex() const { return m_EntityIndex; }
	private:
		EntityStorage& m_Storage;
		size_t m_EntityIndex;
	};
}