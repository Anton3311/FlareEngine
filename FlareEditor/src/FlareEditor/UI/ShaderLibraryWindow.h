#pragma once

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	class ShaderLibraryWindow
	{
	public:
		void OnRenderImGui();

		static void Show();
		static ShaderLibraryWindow& GetInstance();
	private:
		void RenderShaderItem(AssetHandle handle);
	private:
		bool m_Show = false;
	};
}
