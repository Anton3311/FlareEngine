#pragma once

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Transform.h"

#include "FlareECS/World.h"

namespace Flare
{
	class EntityProperties
	{
	public:
		EntityProperties(World& world);

		void OnRenderImGui(Entity entity);
	private:
		void RenderCameraComponent(CameraComponent& cameraComponent);
		void RenderTransformComponent(TransformComponent& transform);
		void RenderSpriteComponent(SpriteComponent& sprite);
		void RenderMeshComponent(MeshComponent& mesh);
		void RenderEnvironmentComponent(Environment& environment);

		void EntityProperties::RenderAddComponentMenu(Entity entity);
	private:
		World& m_World;
	};
}