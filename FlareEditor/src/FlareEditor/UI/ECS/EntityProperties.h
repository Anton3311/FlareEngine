#pragma once

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	struct CameraComponent;
	struct Environment;
	struct SpriteComponent;
	struct TransformComponent;
	class World;

	class EntityProperties
	{
	public:
		EntityProperties(World& world);

		void OnRenderImGui(Entity entity);
	private:
		void RenderCameraComponent(CameraComponent& cameraComponent);
		void RenderTransformComponent(TransformComponent& transform);
		void RenderSpriteComponent(SpriteComponent& sprite);
		void RenderEnvironmentComponent(Environment& environment);

		void EntityProperties::RenderAddComponentMenu(Entity entity);
	private:
		World& m_World;
	};
}