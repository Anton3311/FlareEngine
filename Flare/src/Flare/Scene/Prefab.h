#pragma once

#include "Flare/AssetManager/Asset.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	class FLARE_API Prefab : public Asset
	{
	public:
		Prefab(std::vector<ComponentId>&& components, const uint8_t* prefabData)
			: Asset(AssetType::Prefab), m_Data(prefabData), m_Archetype(INVALID_ARCHETYPE_ID), m_Components(std::move(components)) {}
		Prefab(ArchetypeId archetype, const uint8_t* prefabData)
			: Asset(AssetType::Prefab), m_Data(prefabData), m_Archetype(archetype) {}

		~Prefab()
		{
			if (m_Data != nullptr)
			{
				delete[] m_Data;
				m_Data = nullptr;
			}
		}
	
		Entity CreateInstance(World& world);
	private:
		ArchetypeId m_Archetype;
		std::vector<ComponentId> m_Components;
		const uint8_t* m_Data;
	};
}