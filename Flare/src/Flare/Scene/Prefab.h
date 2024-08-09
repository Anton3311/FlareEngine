#pragma once

#include "Flare/AssetManager/Asset.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/Entity.h"
#include "FlareECS/Commands/Command.h"

namespace Flare
{
	//
	// PrefabHierarchy
	//

	class FLARE_API PrefabHierarchy
	{
	public:
		struct Node
		{
			size_t DataOffset = 0;
			size_t Size = 0;
			ArchetypeId Archetype = INVALID_ARCHETYPE_ID;
		};

		PrefabHierarchy(const Components& compatibleComponents, const Archetypes& compatibleArchetypes);
		~PrefabHierarchy();

		void CopyFromWorld(const World& world);
	private:
		void Release();
		void ReleaseEntityData();
	private:
		uint8_t* m_Buffer = nullptr;
		size_t m_BufferSize = 0;

		const Archetypes& m_CompatibleArchetypes;
		const Components& m_CompatibleComponentsRegistry;

		std::vector<Node> m_Nodes;
	};

	//
	// Prefab
	//

	class FLARE_API Prefab : public Asset
	{
	public:
		FLARE_ASSET;
		FLARE_SERIALIZABLE;

		Prefab(const uint8_t* prefabData,
			const Components& compatibleComponentsRegistry,
			const Archetypes& compatibleArchetypes,
			std::vector<std::pair<ComponentId, void*>>&& components);	
		~Prefab();
	
		Entity CreateInstance(World& world);

		inline PrefabHierarchy& GetHierarchy() { return m_Hierarchy; }
		inline const PrefabHierarchy& GetHierarchy() const { return m_Hierarchy; }
	private:
		std::vector<std::pair<ComponentId, void*>> m_Components;

		PrefabHierarchy m_Hierarchy;

		const Components& m_CompatibleComponentsRegistry;
		const Archetypes& m_CompatibleArchetypes;
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