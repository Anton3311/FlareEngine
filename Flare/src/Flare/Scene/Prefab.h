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
			static constexpr size_t INVALID_PARENT_NODE = SIZE_MAX;

			size_t DataOffset = 0;
			size_t Size = 0;
			ArchetypeId Archetype = INVALID_ARCHETYPE_ID;

			size_t ParentNode = INVALID_PARENT_NODE;
		};

		PrefabHierarchy(const Components& compatibleComponents, Archetypes& compatibleArchetypes);
		~PrefabHierarchy();

		void CopyFromWorld(const World& world);
		void AddEntity(ArchetypeId archetype, size_t parentIndex);

		uint8_t* GetEntityData(size_t nodeIndex) const;

		void EnsureAllocated();

		// Initializes all the components of each entity using a default constructor
		//
		// It is not save to call it multiple times or if any of the components were initialized externally
		// by accessing and writing to directly entity data obtained from GetEntityData
		void InitializeEntities();

		inline const std::vector<Node>& GetNodes() const { return m_Nodes; }
		inline bool IsEmpty() const { return m_Buffer == nullptr && m_BufferSize == 0; }

		inline const Components& GetCompatibleComponents() const { return m_CompatibleComponentsRegistry; }
		inline Archetypes& GetCompatibleArchetypes() const { return m_CompatibleArchetypes; }

		template<typename T>
		T* TryGetNodeComponent(size_t nodeIndex)
		{
			uint8_t* nodeData = GetEntityData(nodeIndex);

			std::optional<size_t> componentOffset = GetNodeComponentOffset(nodeIndex, COMPONENT_ID(T));
			if (!componentOffset)
				return nullptr;

			return (T*)(nodeData + *componentOffset);
		}
	private:
		std::optional<size_t> GetNodeComponentOffset(size_t nodeIndex, ComponentId component) const;
	private:
		void Release();
		void ReleaseEntityData();
	private:
		uint8_t* m_Buffer = nullptr;
		size_t m_BufferSize = 0;

		Archetypes& m_CompatibleArchetypes;
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

		Prefab(const Components& compatibleComponentsRegistry, Archetypes& compatibleArchetypes);
	
		Entity CreateInstance(World& world);

		inline PrefabHierarchy& GetHierarchy() { return m_Hierarchy; }
		inline const PrefabHierarchy& GetHierarchy() const { return m_Hierarchy; }
	private:
		Entity InstantiateHierarchy(World& world) const;
	private:
		PrefabHierarchy m_Hierarchy;

		const Components& m_CompatibleComponentsRegistry;
		const Archetypes& m_CompatibleArchetypes;
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