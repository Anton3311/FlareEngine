#pragma once

#include "Flare/Scene/Components.h"

#include "FlareScriptingCore/ScriptingType.h"
#include "FlareScriptingCore/TypeSerializationSettings.h"

#include <unordered_map>
#include <functional>

namespace Flare
{
	class PropertiesWindow
	{
	public:
		void OnImGuiRender();
	private:
		void RenderAddComponentMenu(Entity entity);

		void RenderCameraComponent(CameraComponent& cameraComponent);
		void RenderTransformComponent(TransformComponent& transform);
		void RenderSpriteComponent(SpriteComponent& sprite);
	};
}