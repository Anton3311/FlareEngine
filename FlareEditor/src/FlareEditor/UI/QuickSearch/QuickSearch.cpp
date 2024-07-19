#include "QuickSearch.h"

#include "Flare/Core/Application.h"
#include "Flare/AssetManager/AssetManager.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"
#include "FlareEditor/UI/EditorGUI.h"

#include <wchar.h>

#include "FlareEditor/ImGui/ImGuiLayer.h"

namespace Flare
{
	static QuickSearch* s_Instance = nullptr;

	QuickSearch::QuickSearch()
		: m_Show(false)
	{
		s_Instance = this;
		Close();
		ClearResult();
	}

	QuickSearch::~QuickSearch()
	{
		s_Instance = nullptr;
	}

	void QuickSearch::OnImGuiRender()
	{
		if (!m_Show)
			return;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

		ImGui::Begin("Quick Search", &m_Show, windowFlags);

		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
			Close();

		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		if (EditorGUI::TextField((uint64_t)"Search", m_CurrentInput))
			CollectMatchedAssets();

		ImGui::PopItemWidth();

		if (ImGui::BeginChild("SearchResultList"))
		{
			std::optional<AssetHandle> selectedHandle = {};

			ImGuiStyle& style = ImGui::GetStyle();

			float padding = style.FramePadding.y;
			float buttonHeight = ImGui::GetFontSize() + 2 * padding;

			ImVec2 areaSize = ImGui::GetContentRegionAvail();
			ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;

			ImU32 textColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Text]);
			ImU32 hoveredColor = ImGui::ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonHovered]);
			
			for (size_t itemIndex = 0; itemIndex < m_AssetSearchResult.size(); itemIndex++)
			{
				AssetHandle handle = m_AssetSearchResult[itemIndex];

				std::string path = AssetManager::GetAssetMetadata(handle)->Path.string();
				if (path.empty())
					continue;

				ImVec2 size = ImVec2(areaSize.x, buttonHeight);
				if (ImGui::InvisibleButton(path.c_str(), size))
					selectedHandle = handle;

				ImRect buttonRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };

				if (ImGui::IsItemHovered() || ImGui::IsItemActive())
					drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, hoveredColor, style.FrameRounding);

				drawList->AddText(buttonRect.Min + ImVec2(padding, padding), textColor, path.c_str(), path.c_str() + path.size());
			}

			if (selectedHandle)
			{
				m_AssetResult.Handle = selectedHandle.value();
				m_AssetResult.Type = AssetSearchResult::ResultType::Ok;
				Close();
			}

			ImGui::EndChild();
		}

		ImGui::End();
	}

	void QuickSearch::FindAsset(UUID resultToken)
	{
		m_ResultToken = resultToken;
		m_Show = true;
		m_CurrentInput.clear();

		CollectMatchedAssets();
	}

	std::optional<QuickSearch::AssetSearchResult> QuickSearch::AcceptAssetResult(UUID resultToken)
	{
		if (resultToken == m_ResultToken)
		{
			auto result = m_AssetResult;
			switch (m_AssetResult.Type)
			{
			case AssetSearchResult::ResultType::None:
				return {};
			case AssetSearchResult::ResultType::NotFound:
			case AssetSearchResult::ResultType::Ok:
			{
				ClearResult();
				break;
			}
			}

			return result;
		}

		return {};
	}

	QuickSearch& QuickSearch::GetInstance()
	{
		FLARE_CORE_ASSERT(s_Instance);
		return *s_Instance;
	}

	void QuickSearch::CollectMatchedAssets()
	{
		Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
		const auto& assetRegistry = assetManager->GetRegistry();

		m_AssetSearchResult.clear();

		for (const auto& [key, entry] : assetRegistry.GetEntries())
		{
			const wchar_t* path = entry.Metadata.Path.c_str();
			size_t pathLength = wcslen(path);

			if (pathLength < m_CurrentInput.size())
				continue;

			bool contains = false;
			for (size_t i = 0; i < pathLength - m_CurrentInput.size() + 1; i++)
			{
				contains = true;
				for (size_t j = 0; j < m_CurrentInput.size(); j++)
				{
					wchar_t chr = 0;
					mbstowcs(&chr, &m_CurrentInput[j], 1);

					if (chr != path[i + j])
					{
						contains = false;
						break;
					}
				}

				if (contains)
					break;
			}

			if (contains)
				m_AssetSearchResult.push_back(key);
		}
	}

	void QuickSearch::Close()
	{
		m_CurrentInput.clear();
		m_Show = false;
	}

	void QuickSearch::ClearResult()
	{
		m_ResultToken = 0;
		m_AssetResult.Handle = NULL_ASSET_HANDLE;
		m_AssetResult.Type = AssetSearchResult::ResultType::None;
	}
}
