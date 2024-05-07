#pragma once

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Core/Signal.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <optional>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <functional>

namespace Flare
{
	class AssetManagerWindow
	{
	private:
		struct AssetTreeNode
		{
			bool IsDirectory;
			bool IsImported;

			std::string Name;
			std::filesystem::path Path;

			AssetHandle Handle;
			uint32_t ChildrenCount;
			uint32_t LastChildIndex;

			AssetTreeNode(const std::string& name, const std::filesystem::path& path)
				: Name(name), Path(path), IsDirectory(true), IsImported(false), Handle(NULL_ASSET_HANDLE), ChildrenCount(0), LastChildIndex(UINT32_MAX) {}
			AssetTreeNode(const std::string& name, const std::filesystem::path& path, AssetHandle handle)
				: Name(name), Path(path), IsDirectory(false), IsImported(false), Handle(handle), ChildrenCount(0), LastChildIndex(UINT32_MAX) {}
		};
	public:
		void OnImGuiRender();

		void RebuildAssetTree();
		void Uninitialize();

		void SetOpenAction(AssetType assetType, const std::function<void(AssetHandle)>& action);
		void ClearOpenActions();
	private:
		using FileNameCallback = std::function<void(std::string_view)>;

		void RenderDirectory();
		void RenderFile();
		void RenderAssetItem(AssetTreeNode* node, AssetHandle handle);

		void BuildDirectory(uint32_t parentIndex, const std::filesystem::path& path);

		void OnOpenFile(AssetHandle handle);

		void ShowCreateNewFilePopup(const FileNameCallback& callback);
		void RenderCreateNewFilePopup();

		void RenderFileOrDirectoryMenuItems(const AssetTreeNode& node);
		void RenderCreateAssetMenuItems(const AssetTreeNode& rootNode);
	public:
		Signal<AssetHandle> OnAssetSelectionChanged;
	private:
		Ref<EditorAssetManager> m_AssetManager;

		uint32_t m_NodeRenderIndex;

		// Files and subdirectories are stored after the parent direcetory in the array
		std::vector<AssetTreeNode> m_AssetTree;

		std::unordered_map<AssetType, std::function<void(AssetHandle)>> m_FileOpenActions;
		const char* m_FileNamePopupId = "Enter name";

		bool m_ShowNewFilePopup = false;
		char m_TextInputBuffer[512];
		std::function<void(std::string_view fileName)> m_OnNewFileNameCallback;
	};
}