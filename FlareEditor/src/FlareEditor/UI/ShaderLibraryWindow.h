#pragma once

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	struct ComputeShaderMetadata;
	struct GraphicsShaderMetadata;
	struct ShaderMetadata;

	class ShaderLibraryWindow
	{
	public:
		void OnRenderImGui();

		static void Show();
		static ShaderLibraryWindow& GetInstance();
	private:
		void RenderShaderAssetMetadata(const AssetMetadata* metadata);
		void RenderGraphicsShaderMetadata(Ref<const GraphicsShaderMetadata> metadata);
		void RenderComputeShaderMetadata(Ref<const ComputeShaderMetadata> metadata);
		void RenderShaderMetadata(Ref<const ShaderMetadata> metadata);
	private:
		bool m_Show = false;
		AssetHandle m_SelectedShader = NULL_ASSET_HANDLE;
	};
}
