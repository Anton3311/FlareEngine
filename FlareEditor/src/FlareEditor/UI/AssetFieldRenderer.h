#pragma once

#include "Flare/AssetManager/Asset.h"
#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
	class AssetFieldRenderer
	{
	public:
		AssetFieldRenderer(uint64_t widgetId, AssetHandle& handle, const AssetDescriptor* descriptor = nullptr)
			: m_WidgetId(widgetId), m_Handle(&handle), m_Asset(nullptr), m_AssetDescriptor(descriptor) {}

		AssetFieldRenderer(uint64_t widgetId, Ref<Asset>& asset, const AssetDescriptor& assetDescriptor)
			: m_WidgetId(widgetId), m_Handle(nullptr), m_Asset(&asset), m_AssetDescriptor(&assetDescriptor) {}

		bool OnRenderImGui();
	private:
		uint64_t GetNextWidgetId();

		void RenderAssetPreview(ImVec2 previewSize, ImVec2 previewPosition, bool hovered);
		void GetValidHandle();
		bool SetAssetResult(const Ref<Asset>& asset);
		void ResetValue();
	private:
		uint64_t m_WidgetId;
		std::optional<AssetHandle> m_ValidHandle;

		AssetHandle* m_Handle = nullptr;
		Ref<Asset>* m_Asset = nullptr;

		const AssetMetadata* m_AssetMetadata = nullptr;
		const AssetDescriptor* m_AssetDescriptor = nullptr;
	};
}
