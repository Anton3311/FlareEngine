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
		SpritesRendererSystem();

		virtual void OnConfig(SystemConfig& config) override {}
		virtual void OnUpdate(SystemExecutionContext& context) override;
	private:
		void RenderQuads(SystemExecutionContext& context);
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
}