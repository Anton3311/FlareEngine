#pragma once

#include "Flare/AssetManager/Asset.h"
#include "FlarePlatform/Event.h"

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

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnEvent(Event& event) = 0;
	protected:
		virtual void OnOpen(AssetHandle asset) = 0;
		virtual void OnClose() = 0;
		virtual void OnRenderImGui(bool& show) = 0;
	private:
		bool m_Show;
	};
}