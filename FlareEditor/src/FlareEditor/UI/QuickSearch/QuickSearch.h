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
		struct AssetSearchResult
		{
			enum class ResultType
			{
				None,
				Ok,
				NotFound,
			};

			AssetHandle Handle;
			ResultType Type;
		};

		QuickSearch();
		~QuickSearch();

		void OnImGuiRender();

		void FindAsset(UUID resultToken);
		std::optional<AssetSearchResult> AcceptAssetResult(UUID resultToken);
	public:
		static QuickSearch& GetInstance();
	private:
		void CollectMatchedAssets();

		void Close();
		void ClearResult();
	private:
		bool m_Show;
		std::string m_CurrentInput;

		std::vector<AssetHandle> m_AssetSearchResult;

		AssetSearchResult m_AssetResult;
		UUID m_ResultToken;
	};
}