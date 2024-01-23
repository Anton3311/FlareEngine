#pragma once

#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Scene/Components.h"

#include "FlareECS/World.h"
#include "FlareECS/System/SystemInitializer.h"

namespace Flare
{
	struct SpritesRendererSystem : public System
	{
	public:
		virtual void OnConfig(World& world, SystemConfig& config) override;
		virtual void OnUpdate(World& world, SystemExecutionContext& context) override;
	private:
		void RenderQuads(World& world, SystemExecutionContext& context);
		void RenderText(SystemExecutionContext& context);
	private:
		struct EntityQueueElement
		{
			Entity Id;
			int32_t SortingLayer;
			AssetHandle Material;
		};

		Query m_SpritesQuery;
		Query m_TextQuery;
		std::vector<EntityQueueElement> m_SortedEntities;
	};

	struct MeshesRendererSystem : public System
	{
	public:
		void OnConfig(World& world, SystemConfig& config) override;
		void OnUpdate(World& world, SystemExecutionContext& context) override;
	private:
		Query m_Query;
		Query m_DecalsQuery;
	};
}