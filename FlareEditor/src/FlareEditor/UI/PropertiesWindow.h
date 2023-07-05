#pragma once

#include "Flare/Scene/Components.h"

#include <unordered_map>
#include <functional>

namespace Flare
{
	class PropertiesWindow
	{
	public:
		void OnImGuiRender();
	private:
		void RenderCameraComponent(CameraComponent& cameraComponent);
	};
}