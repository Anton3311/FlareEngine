#pragma once

#include "Flare/Scene/Components.h"

#include "FlareEditor/AssetManager/TextureImporter.h"
#include "FlareEditor/UI/AssetManagerWindow.h"

#include <unordered_map>
#include <functional>

namespace Flare
{
	class PropertiesWindow
	{
	public:
		PropertiesWindow(AssetManagerWindow& assetManagerWindow);

		void OnAttach();
		void OnImGuiRender();
	private:
		void RenderEntityProperties(World& world, Entity entity);
		void RenderAssetProperties(AssetHandle handle);

		void RenderAddComponentMenu(Entity entity);

		void RenderCameraComponent(CameraComponent& cameraComponent);
		void RenderTransformComponent(TransformComponent& transform);
		void RenderSpriteComponent(SpriteComponent& sprite);

		bool RenderTextureImportSettingsEditor(AssetHandle handle, TextureImportSettings& importSettings);
	private:
		AssetManagerWindow& m_AssetManagerWindow;
	};
}