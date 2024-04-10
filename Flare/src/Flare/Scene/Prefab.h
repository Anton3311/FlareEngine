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
		FLARE_ASSET;
		FLARE_SERIALIZABLE;

		Prefab(const uint8_t* prefabData, const Components* compatibleComponentsRegistry, std::vector<std::pair<ComponentId, void*>>&& components);	
		~Prefab();
	
		Entity CreateInstance(World& world);
	private:
		std::vector<std::pair<ComponentId, void*>> m_Components;
		const Components* m_CompatibleComponentsRegistry = nullptr;
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