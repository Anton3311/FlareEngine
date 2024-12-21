#pragma once

#include "Flare/AssetManager/Asset.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/Entity.h"
#include "FlareECS/Commands/Command.h"

namespace Flare
{
	enum class PrefabInstantiationFlags
	{
		None = 0,
		AddMetadataComponents = 1,
	};

	FLARE_IMPL_ENUM_BITFIELD(PrefabInstantiationFlags);

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
		// It is not safe to call it multiple times or if any of the components were initialized externally
		// by accessing and writing directly to entity data obtained from GetEntityData
		void InitializeEntities();

		inline const std::vector<Node>& GetNodes() const { return m_Nodes; }
		inline bool IsEmpty() const { return m_Buffer == nullptr && m_BufferSize == 0; }

		inline const Components& GetCompatibleComponents() const { return m_CompatibleComponentsRegistry; }
		inline Archetypes& GetCompatibleArchetypes() const { return m_CompatibleArchetypes; }

		template<typename T>
		T* TryGetNodeComponent(size_t nodeIndex)
		{
			FLARE_PROFILE_FUNCTION();
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

	enum class PrefabFlags
	{
		None = 0,
		Generated = 1,
	};

	FLARE_IMPL_ENUM_BITFIELD(PrefabFlags);

	class FLARE_API Prefab : public Asset
	{
	public:
		FLARE_ASSET;
		FLARE_SERIALIZABLE;

		Prefab(const Components& compatibleComponentsRegistry, Archetypes& compatibleArchetypes, PrefabFlags flags);
		Prefab(const Components& compatibleComponentsRegistry, Archetypes& compatibleArchetypes, PrefabFlags flags, AssetHandle sourceMesh);
	
		Entity CreateInstance(World& world, PrefabInstantiationFlags instantiationFlags = PrefabInstantiationFlags::None);
		std::optional<Entity> TryCreateInstance(World& world, PrefabInstantiationFlags instantiationFlags = PrefabInstantiationFlags::None);

		inline PrefabHierarchy& GetHierarchy() { return m_Hierarchy; }
		inline const PrefabHierarchy& GetHierarchy() const { return m_Hierarchy; }

		constexpr PrefabFlags GetFlags() const { return m_Flags; }
		constexpr AssetHandle GetSourceMesh() const { return m_SourceMesh; }
	public:

		// Creates a new prefab with a single entity, that has a transform
		static Ref<Prefab> CreateEmpty(Archetypes& compatibleArchetypes);
	private:
		Entity InstantiateHierarchy(World& world, PrefabInstantiationFlags instantiationFlags) const;
	private:
		PrefabFlags m_Flags;
		PrefabHierarchy m_Hierarchy;

		AssetHandle m_SourceMesh = NULL_ASSET_HANDLE;

		const Components& m_CompatibleComponentsRegistry;
		const Archetypes& m_CompatibleArchetypes;
	};
	
	class FLARE_API InstantiatePrefab : public EntityCommand
	{
	public:
		InstantiatePrefab() = default;
		InstantiatePrefab(const Ref<Prefab>& prefab);
		InstantiatePrefab(const Ref<Prefab>& prefab, PrefabInstantiationFlags instantiationFlags);

		virtual void Apply(CommandContext& context, World& world) override;
		virtual void Initialize(FutureEntity entity);
	private:
		FutureEntity m_OutputEntity;
		Ref<Prefab> m_Prefab;
		PrefabInstantiationFlags m_InstantiationFlags;
	};

	struct FLARE_API PrefabInstance
	{
		FLARE_COMPONENT;

		AssetHandle PrefabHandle;
	};
}