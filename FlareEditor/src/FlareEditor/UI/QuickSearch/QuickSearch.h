#pragma once

#include "Flare/AssetManager/Asset.h"

#include <optional>
#include <vector>
#include <string>

namespace Flare
{
	class QuickSearch
	{
	public:
		using AssetSearchCallback = std::function<void(AssetHandle)>;

		QuickSearch();
		~QuickSearch();

		void OnImGuiRender();

		void FindAsset(const AssetSearchCallback& callback);
	public:
		static QuickSearch& GetInstance();
	private:
		void Close();
	private:
		bool m_Show;
		std::string m_CurrentInput;

		std::vector<AssetHandle> m_AssetSearchResult;
		AssetSearchCallback m_Callback;
	};
}