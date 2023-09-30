#pragma once

#include "Flare/Scene/Components.h"

#include "FlareECS/World.h"

namespace Flare
{
	class EntityProperties
	{
	public:
		EntityProperties(World& world);

		void OnRenderImGui(Entity entity);
	private:
		void EntityProperties::RenderCameraComponent(CameraComponent& cameraComponent);
		void EntityProperties::RenderTransformComponent(TransformComponent& transform);
		void EntityProperties::RenderSpriteComponent(SpriteComponent& sprite);

		void EntityProperties::RenderAddComponentMenu(Entity entity);
	private:
		World& m_World;
	};
}