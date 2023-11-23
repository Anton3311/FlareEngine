#include "AssetManagerWindow.h"

#include "FlareCore/Assert.h"

#include "Flare/Project/Project.h"

#include "Flare/Renderer/Material.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"

#include "FlareEditor/AssetManager/MaterialImporter.h"

#include <fstream>
#include <imgui.h>

namespace Flare
{
	void AssetManagerWindow::OnImGuiRender()
	{
		ImGui::Begin("Asset Manager");

		if (ImGui::Button("Refresh"))
			RebuildAssetTree();

		ImGui::SameLine();

		ImGuiStyle& style = ImGui::GetStyle();

		const float maxTreeWidth = 300.0f;

		ImVec2 treeSize = ImGui::GetContentRegionAvail();
		treeSize.x = glm::min(maxTreeWidth, treeSize.x);
		
		if (EditorGUI::BeginToggleGroup("Mode", 2))
		{
			if (EditorGUI::ToggleGroupItem("All", m_Mode == AssetTreeViewMode::All))
				m_Mode = AssetTreeViewMode::All;
			if (EditorGUI::ToggleGroupItem("Registry", m_Mode == AssetTreeViewMode::Registry))
				m_Mode = AssetTreeViewMode::Registry;

			EditorGUI::EndToggleGroup();
		}

		ImGui::Separator();
		
		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 14.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));

		if (ImGui::BeginChild("Tree", treeSize))
		{
			m_NodeRenderIndex = 0;
			if (m_AssetTree.size())
				RenderDirectory();
			ImGui::EndChild();
		}

		RenderCreateNewFilePopup();

		ImGui::PopStyleVar();
		ImGui::PopStyleVar();

		ImGui::End();
	}

	void AssetManagerWindow::RebuildAssetTree()
	{
		if (m_AssetManager == nullptr)
			m_AssetManager = As<EditorAssetManager>(AssetManager::GetInstance());

		m_AssetTree.clear();

		uint32_t rootIndex = 0;
		std::filesystem::path root = AssetRegistrySerializer::GetAssetsRoot();
		m_AssetTree.emplace_back("Assets", root);

		BuildDirectory(rootIndex, root);
	}

	void AssetManagerWindow::SetOpenAction(AssetType assetType, const std::function<void(AssetHandle)>& action)
	{
		m_FileOpenActions[assetType] = action;
	}

	void AssetManagerWindow::RenderDirectory()
	{
		const AssetTreeNode& node = m_AssetTree[m_NodeRenderIndex];
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;

		bool opened = ImGui::TreeNodeEx(node.Name.c_str(), flags, "%s", node.Name.c_str());
		if (ImGui::BeginPopupContextItem(node.Name.c_str()))
		{
			if (ImGui::BeginMenu("New"))
			{
				if (ImGui::MenuItem("Prefab"))
				{
					m_ShowNewFileNamePopup = true;
					m_OnNewFileNameEntered = [this, nodeIndex = m_NodeRenderIndex](std::string_view name)
					{
						FLARE_CORE_ASSERT(m_AssetTree[nodeIndex].IsDirectory);
						std::filesystem::path path = m_AssetTree[nodeIndex].Path / name;
						path.replace_extension(".flrprefab");

						if (!std::filesystem::exists(path))
						{
							std::ofstream output(path);
							output << R"(Components:
  - Name: struct Flare::TransformComponent
    Position: [0, 0, 0]
    Rotation: [0, 0, 0]
    Scale: [1, 1, 1])";
						}
					};
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (opened)
		{
			for (uint32_t i = 0; i < node.ChildrenCount; i++)
			{
				m_NodeRenderIndex++;

				if (m_NodeRenderIndex >= m_AssetTree.size())
					break;

				if (m_AssetTree[m_NodeRenderIndex].IsDirectory)
					RenderDirectory();
				else
					RenderFile();
			}

			ImGui::TreePop();
		}

		if (node.ChildrenCount != 0)
			m_NodeRenderIndex = node.LastChildIndex;
	}

	void AssetManagerWindow::RenderFile()
	{
		AssetTreeNode& node = m_AssetTree[m_NodeRenderIndex];

		if (m_Mode == AssetTreeViewMode::Registry && !node.IsImported)
			return;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;

		const AssetMetadata* metadata = AssetManager::GetAssetMetadata(node.Handle);
		if (metadata == nullptr || metadata->SubAssets.size() == 0)
			flags |= ImGuiTreeNodeFlags_Leaf;

		if (!node.IsImported)
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));

		bool opened = ImGui::TreeNodeEx(node.Name.c_str(), flags, node.Name.c_str());

		if (!node.IsImported)
			ImGui::PopStyleColor();

		if (node.IsImported && ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(ASSET_PAYLOAD_NAME, &node.Handle, sizeof(AssetHandle));
			ImGui::EndDragDropSource();
		}

		if (!node.IsDirectory)
		{
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				OnOpenFile(node.Handle);
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			EditorLayer::GetInstance().Selection.SetAsset(node.Handle);
			OnAssetSelectionChanged.Invoke(node.Handle);
		}

		if (opened)
		{
			if (ImGui::BeginPopupContextItem(node.Name.c_str()))
			{
				if (!node.IsDirectory && node.IsImported && ImGui::MenuItem("Open"))
					OnOpenFile(node.Handle);

				if (!node.IsDirectory && node.IsImported)
				{
					FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(node.Handle));
					if (ImGui::MenuItem("Remove"))
					{
						m_AssetManager->RemoveFromRegistry(node.Handle);
						node.IsImported = false;
					}

					if (ImGui::MenuItem("Reload"))
					{
						if (AssetManager::IsAssetLoaded(node.Handle))
							m_AssetManager->ReloadAsset(node.Handle);
					}
				}

				if (ImGui::BeginMenu("New"))
				{
					if (node.IsImported)
					{
						FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(node.Handle));
						const AssetMetadata* metadata = AssetManager::GetAssetMetadata(node.Handle);

						if (metadata->Type == AssetType::Shader && ImGui::MenuItem("Material"))
						{
							m_ShowNewFileNamePopup = true;
							m_OnNewFileNameEntered = [this, nodeIndex = m_NodeRenderIndex](std::string_view name)
							{
								FLARE_CORE_ASSERT(!m_AssetTree[nodeIndex].IsDirectory);
								std::filesystem::path path = m_AssetTree[nodeIndex].Path.parent_path() / name;
								path.replace_extension(".flrmat");

								Ref<Material> material = CreateRef<Material>(m_AssetTree[nodeIndex].Handle);
								MaterialImporter::SerializeMaterial(material, path);

								m_AssetManager->ImportAsset(path, material);
							};
						}
					}

					ImGui::EndMenu();
				}

				if (!node.IsImported && ImGui::MenuItem("Import"))
				{
					node.Handle = m_AssetManager->ImportAsset(node.Path);
					node.IsImported = node.Handle != NULL_ASSET_HANDLE;
				}

				ImGui::EndMenu();
			}

			if (metadata != nullptr)
			{
				for (AssetHandle handle : metadata->SubAssets)
				{
					const AssetMetadata* meta = AssetManager::GetAssetMetadata(handle);
					if (meta->Name.size() > 0)
						RenderAssetItem(handle);
				}
			}

			ImGui::TreePop();
		}
	}

	void AssetManagerWindow::RenderAssetItem(AssetHandle handle)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;

		const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);

		bool opened = ImGui::TreeNodeEx(metadata->Name.c_str(), flags, metadata->Name.c_str());

		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(ASSET_PAYLOAD_NAME, &handle, sizeof(AssetHandle));
			ImGui::EndDragDropSource();
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			EditorLayer::GetInstance().Selection.SetAsset(handle);
			OnAssetSelectionChanged.Invoke(handle);
		}

		if (opened)
			ImGui::TreePop();
	}

	void AssetManagerWindow::BuildDirectory(uint32_t parentIndex, const std::filesystem::path& path)
	{
		static uint32_t index = 1;
		FLARE_CORE_ASSERT(std::filesystem::is_directory(path));

		for (std::filesystem::path child : std::filesystem::directory_iterator(path))
		{
			if (std::filesystem::is_directory(child))
			{
				m_AssetTree.emplace_back(child.filename().generic_string(), child);
				BuildDirectory((uint32_t) (m_AssetTree.size() - 1), child);
			}
			else
			{
				std::optional<AssetHandle> handle = m_AssetManager->FindAssetByPath(child);

				AssetTreeNode& node = m_AssetTree.emplace_back(child.filename().generic_string(), child, handle.value_or(AssetHandle()));
				node.IsImported = handle.has_value();
			}

			m_AssetTree[parentIndex].LastChildIndex = (uint32_t)(m_AssetTree.size() - 1);
			m_AssetTree[parentIndex].ChildrenCount++;
		}
	}

	void AssetManagerWindow::OnOpenFile(AssetHandle handle)
	{
		if (AssetManager::IsAssetHandleValid(handle))
		{
			const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);
			if (metadata)
			{
				auto action = m_FileOpenActions.find(metadata->Type);
				if (action != m_FileOpenActions.end())
					action->second(metadata->Handle);
			}
		}
	}

	void AssetManagerWindow::RenderCreateNewFilePopup()
	{
		bool createNewFile = false;

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

		if (m_ShowNewFileNamePopup)
		{
			m_ShowNewFileNamePopup = false;
			std::memset(m_TextInputBuffer, 0, sizeof(m_TextInputBuffer));
			ImGui::OpenPopup("EnterNewFileName");
		}

		if (ImGui::BeginPopupModal("EnterNewFileName", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetStyle().FramePadding.y);
			ImGui::Text("Name");
			ImGui::SameLine();

			ImGui::PushID("FileNameField");
			ImGui::InputText("", m_TextInputBuffer, 512);
			ImGui::PopID();

			if (ImGui::Button("Create"))
				createNewFile = true;

			if (ImGui::IsKeyPressed(ImGuiKey_Enter))
				createNewFile = true;
			if (ImGui::IsKeyPressed(ImGuiKey_Escape))
				ImGui::CloseCurrentPopup();

			ImGui::SameLine();

			if (ImGui::Button("Cancel"))
				ImGui::CloseCurrentPopup();

			if (createNewFile)
				ImGui::CloseCurrentPopup();

			ImGui::EndPopup();
		}

		if (createNewFile)
		{
			std::string_view fileName = m_TextInputBuffer;

			if (m_OnNewFileNameEntered)
			{
				m_OnNewFileNameEntered(fileName);
				m_OnNewFileNameEntered = nullptr;
			}
		}
	}
}
