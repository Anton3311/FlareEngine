#include "AssetManagerWindow.h"

#include "Flare/Core/Assert.h"

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
		
		if (ImGui::BeginCombo("Mode", m_Mode == AssetTreeViewMode::All ? "All" : "Registry"))
		{
			{
				bool selected = m_Mode == AssetTreeViewMode::All;
				if (ImGui::Selectable("All", selected))
					m_Mode = AssetTreeViewMode::All;

				if (selected)
					ImGui::SetItemDefaultFocus();
			}

			{
				bool selected = m_Mode == AssetTreeViewMode::Registry;
				if (ImGui::Selectable("Registry", selected))
					m_Mode = AssetTreeViewMode::Registry;

				if (selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		ImGui::Separator();

		ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 14.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));

		if (ImGui::BeginChild("Tree"))
		{
			m_NodeRenderIndex = 0;
			if (m_AssetTree.size())
				RenderDirectory();
			ImGui::EndChild();
		}

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
		m_AssetTree.emplace_back("Assets", m_AssetManager->GetRoot());

		BuildDirectory(rootIndex, m_AssetManager->GetRoot());
	}

	void AssetManagerWindow::RenderDirectory()
	{
		const AssetTreeNode& node = m_AssetTree[m_NodeRenderIndex];

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
		bool opened = ImGui::TreeNodeEx(node.Name.c_str(), flags, "%s", node.Name.c_str());

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

		m_NodeRenderIndex = node.LastChildIndex;
	}

	void AssetManagerWindow::RenderFile()
	{
		AssetTreeNode& node = m_AssetTree[m_NodeRenderIndex];

		if (m_Mode == AssetTreeViewMode::Registry && !node.IsImported)
			return;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_SpanFullWidth;
		if (ImGui::TreeNodeEx(node.Name.c_str(), flags, "%s", node.Name.c_str()))
		{
			if (m_Mode == AssetTreeViewMode::All)
			{
				if (ImGui::BeginPopupContextWindow(node.Name.c_str()))
				{
					if (ImGui::MenuItem("Import"))
						node.IsImported = m_AssetManager->ImportAsset(node.Path);

					ImGui::EndMenu();
				}
			}

			ImGui::TreePop();
		}

		if (m_Mode == AssetTreeViewMode::Registry && ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload("ASSET_HANDLE", &node.Handle, sizeof(AssetHandle));
			ImGui::EndDragDropSource();
		}
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
}
