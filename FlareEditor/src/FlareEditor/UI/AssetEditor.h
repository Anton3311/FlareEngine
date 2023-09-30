#pragma once

#include "Flare/AssetManager/Asset.h"

namespace Flare
{
	class AssetEditor
	{
	public:
		AssetEditor()
			: m_Show(false) {}

		virtual ~AssetEditor() = default;
	public:
		inline bool IsVisible() const { return m_Show; }
		void Open(AssetHandle asset);
		void Close();
		void OnUpdate();
	protected:
		virtual void OnOpen(AssetHandle asset) = 0;
		virtual void OnClose() = 0;
		virtual void OnRenderImGui(bool& show) = 0;
	private:
		bool m_Show;
	};
}