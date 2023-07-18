#pragma once

#include "FlareScriptingCore/Bindings/ECS/ECS.h"
#include "FlareScriptingCore/Bindings/ECS/Component.h"

namespace Flare::Internal
{
	struct EntityViewBindings
	{
		static EntityViewBindings Bindings;
	};

	class EntityElement
	{
	public:
		constexpr EntityElement(uint8_t* data)
			: m_Data(data) {}

		constexpr uint8_t* GetData() { return m_Data; }
		constexpr const uint8_t* GetData() const { return m_Data; }
	private:
		uint8_t* m_Data;
	};

	class EntityViewIterator
	{
	public:
		constexpr EntityViewIterator(uint8_t* storagePointer, size_t entitySize)
			: m_StoragePointer(storagePointer), m_EntitySize(entitySize) {}

		constexpr EntityElement operator*()
		{
			return EntityElement(m_StoragePointer);
		}

		constexpr EntityViewIterator operator++()
		{
			m_StoragePointer += m_EntitySize;
			return *this;
		}

		constexpr bool operator==(const EntityViewIterator& other)
		{
			return m_StoragePointer == other.m_StoragePointer && m_EntitySize == other.m_EntitySize;
		}

		constexpr bool operator!=(const EntityViewIterator& other)
		{
			return m_StoragePointer != other.m_StoragePointer || m_EntitySize != other.m_EntitySize;
		}
	private:
		uint8_t* m_StoragePointer;
		size_t m_EntitySize;
	};

	class EntityView
	{
	public:
		constexpr EntityView(ArchetypeId archetype, uint8_t* entitiesBuffer, size_t entitySize, size_t entitiesCount)
			: m_Archetype(archetype), m_EntityStorageBuffer(entitiesBuffer), m_EntitySize(entitySize), m_EntitiesCount(entitiesCount) {}

		constexpr size_t GetEntitySize() const { return m_EntitySize; }
		constexpr size_t GetEntitiesCount() const { return m_EntitiesCount; }

		constexpr EntityViewIterator begin()
		{
			return EntityViewIterator(m_EntityStorageBuffer, m_EntitySize);
		}

		constexpr EntityViewIterator end()
		{
			return EntityViewIterator(m_EntityStorageBuffer + m_EntitySize * m_EntitiesCount, m_EntitySize);
		}
	private:
		ArchetypeId m_Archetype;
		uint8_t* m_EntityStorageBuffer;
		size_t m_EntitySize;
		size_t m_EntitiesCount;
	};
}