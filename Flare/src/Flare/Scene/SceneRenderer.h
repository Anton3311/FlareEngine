#pragma once

#include "Flare/Renderer/SceneSubmition.h"

#include "FlareECS/World.h"
#include "FlareECS/System/SystemInitializer.h"

namespace Flare
{
	class Viewport;
	class Scene;
	class FLARE_API SceneRenderer
	{
	public:
		SceneRenderer(Ref<Scene> scene);

		inline Ref<Scene> GetScene() const { return m_Scene; }

		void CollectSceneData();

		// Renders the scene to a given viewport.
		// In case the given view is null, uses the one given by SceneSubmition.
		void RenderViewport(Viewport& viewport, const RenderView* viewOverride = nullptr);
	private:
		void InitializeQueries();

		void PrepareViewportForRendering(Viewport& viewport, const RenderView& view);
	private:
		Query m_CameraQuery;
		Query m_EnvironmentQuery;
		Query m_DirectionalLightQuery;
		Query m_PointLightsQuery;
		Query m_SpotLightsQuery;

		Ref<Scene> m_Scene = nullptr;

		SceneSubmition m_SceneSubmition;
	};

	struct SpriteRendererSystem : public System
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

	struct MeshRendererSystem : public System
	{
	public:
		void OnConfig(World& world, SystemConfig& config) override;
		void OnUpdate(World& world, SystemExecutionContext& context) override;
	private:
		Query m_Query;
	};

	struct DecalRendererSystem : public System
	{
	public:
		void OnConfig(World& world, SystemConfig& config) override;
		void OnUpdate(World& world, SystemExecutionContext& context) override;
	private:
		Query m_DecalsQuery;
	};
}