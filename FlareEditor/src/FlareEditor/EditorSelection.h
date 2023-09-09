#pragma once

#include "Flare/AssetManager/Asset.h"

#include "FlareECS/Entity/Entity.h"

#include <optional>

namespace Flare
{
	enum class EditorSelectionType
	{
		None,
		Entity,
		Asset,
	};

	struct EditorSelection
	{
	public:
		EditorSelection()
			: m_Type(EditorSelectionType::None), m_Entity(Entity()), m_Asset(NULL_ASSET_HANDLE) {}

		EditorSelection(const EditorSelection&) = delete;
		EditorSelection& operator=(const EditorSelection&) = delete;

		constexpr EditorSelectionType GetType() const { return m_Type; }

		constexpr void Reset()
		{
			m_Type = EditorSelectionType::None;
		}

		inline void SetEntity(Entity entity)
		{
			m_Type = EditorSelectionType::Entity;
			m_Entity = entity;
		}

		inline void SetAsset(AssetHandle handle)
		{
			m_Type = EditorSelectionType::Asset;
			m_Asset = handle;
		}

		inline Entity GetEntity() const
		{
			FLARE_CORE_ASSERT(m_Type == EditorSelectionType::Entity);
			return m_Entity;
		}

		inline AssetHandle GetAssetHandle() const
		{
			FLARE_CORE_ASSERT(m_Type == EditorSelectionType::Asset);
			return m_Asset;
		}

		inline std::optional<Entity> TryGetEntity() const
		{
			if (m_Type == EditorSelectionType::Entity)
				return m_Entity;
			return {};
		}

		inline std::optional<AssetHandle> TryGetAsset() const
		{
			if (m_Type == EditorSelectionType::Asset)
				return m_Asset;
			return {};
		}
	private:
		EditorSelectionType m_Type;

		union
		{
			Entity m_Entity;
			AssetHandle m_Asset;
		};
	};
}