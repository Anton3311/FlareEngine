#pragma once

#include "Flare/AssetManager/Asset.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/Entity.h"
#include "FlareECS/Commands/Command.h"

namespace Flare
{
	class FLARE_API Prefab : public Asset
	{
	public:
		Prefab(const uint8_t* prefabData, std::vector<std::pair<ComponentId, const void*>>&& components)
			: Asset(AssetType::Prefab), m_Data(prefabData), m_Components(std::move(components)) {}
		
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
		std::vector<std::pair<ComponentId, const void*>> m_Components;
		const uint8_t* m_Data;
	};
	
	class FLARE_API InstantiatePrefab : public EntityCommand
	{
	public:
		InstantiatePrefab() = default;
		InstantiatePrefab(const Ref<Prefab>& prefab);

		virtual void Apply(CommandContext& context, World& world) override;
		virtual void Initialize(FutureEntity entity);
	private:
		FutureEntity m_OutputEntity;
		Ref<Prefab> m_Prefab;
	};
}