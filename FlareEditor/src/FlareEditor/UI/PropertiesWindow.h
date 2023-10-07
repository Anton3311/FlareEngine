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
		void RenderAssetProperties(AssetHandle handle);

		bool RenderTextureImportSettingsEditor(AssetHandle handle, TextureImportSettings& importSettings);
		bool RenderMaterialEditor(AssetHandle handle);
	private:
		AssetManagerWindow& m_AssetManagerWindow;
	};
}