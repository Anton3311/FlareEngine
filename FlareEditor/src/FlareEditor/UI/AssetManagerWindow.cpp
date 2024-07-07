#include "AssetManagerWindow.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Project/Project.h"

#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "FlareEditor/EditorLayer.h"
#include "FlareEditor/UI/EditorGUI.h"

#include "FlareEditor/AssetManager/MaterialImporter.h"
#include "FlareEditor/AssetManager/SpriteImporter.h"

#include "FlarePlatform/Platform.h"

#include <fstream>
#include <imgui.h>

namespace Flare
{
    void AssetManagerWindow::OnImGuiRender()
    {
        FLARE_PROFILE_FUNCTION();
        ImGui::Begin("Asset Manager");

        if (ImGui::Button("Refresh"))
            RebuildAssetTree();

        ImGuiStyle& style = ImGui::GetStyle();

        const float maxTreeWidth = 300.0f;

        ImVec2 treeSize = ImGui::GetContentRegionAvail();
        treeSize.x = glm::min(maxTreeWidth, treeSize.x);
        
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
        FLARE_PROFILE_FUNCTION();
        if (m_AssetManager == nullptr)
            m_AssetManager = As<EditorAssetManager>(AssetManager::GetInstance());

        m_AssetTree.clear();

        uint32_t rootIndex = 0;
        std::filesystem::path root = EditorAssetManager::GetAssetsRoot();
        m_AssetTree.emplace_back("Assets", root);

        BuildDirectory(rootIndex, root);
    }

    void AssetManagerWindow::Uninitialize()
    {
        FLARE_PROFILE_FUNCTION();
        m_AssetManager = nullptr;
        m_AssetTree.clear();
        m_FileOpenActions.clear();
        m_OnNewFileNameCallback = nullptr;
    }

    void AssetManagerWindow::SetOpenAction(AssetType assetType, const std::function<void(AssetHandle)>& action)
    {
        m_FileOpenActions[assetType] = action;
    }

    void AssetManagerWindow::ClearOpenActions()
    {
        m_FileOpenActions.clear();
    }

    void AssetManagerWindow::RenderDirectory()
    {
        FLARE_PROFILE_FUNCTION();
        const AssetTreeNode& node = m_AssetTree[m_NodeRenderIndex];
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_FramePadding
            | ImGuiTreeNodeFlags_SpanFullWidth;

        bool opened = ImGui::TreeNodeEx(node.Name.c_str(), flags, "%s", node.Name.c_str());
        if (ImGui::BeginPopupContextItem(node.Name.c_str()))
        {
            if (ImGui::BeginMenu("New"))
            {
                if (ImGui::MenuItem("Folder"))
                {
                    ShowCreateNewFilePopup([this, nodeIndex = m_NodeRenderIndex](std::string_view name)
                    {
                        FLARE_CORE_ASSERT(m_AssetTree[nodeIndex].IsDirectory);

                        std::filesystem::path path = m_AssetTree[nodeIndex].Path / name;
                        if (std::filesystem::create_directory(path))
                            FLARE_CORE_ERROR("Failed to create directory: '{}'", path.string());
                        else
                            RebuildAssetTree();
                    });
                }

                ImGui::Separator();

                RenderCreateAssetMenuItems(node);

                ImGui::EndMenu();
            }

            RenderFileOrDirectoryMenuItems(node);

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
        FLARE_PROFILE_FUNCTION();
        AssetTreeNode& node = m_AssetTree[m_NodeRenderIndex];

        if (node.Handle != NULL_ASSET_HANDLE)
        {
            RenderAssetItem(&node, node.Handle);
            return;
        }
        else
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                | ImGuiTreeNodeFlags_FramePadding
                | ImGuiTreeNodeFlags_SpanFullWidth
                | ImGuiTreeNodeFlags_Leaf;

            const auto& editorSelection = EditorLayer::GetInstance().Selection;
            if (editorSelection.GetType() == EditorSelectionType::Asset && node.Handle == editorSelection.GetAssetHandle())
                flags |= ImGuiTreeNodeFlags_Selected;

            const ImGuiStyle& style = ImGui::GetStyle();
            ImGui::PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
			ImGui::TreeNodeEx(node.Name.c_str(), flags, node.Name.c_str());
            ImGui::PopStyleColor();

			if (ImGui::BeginPopupContextItem(node.Name.c_str()))
			{
				if (ImGui::MenuItem("Import"))
				{
					node.Handle = m_AssetManager->ImportAsset(node.Path, NULL_ASSET_HANDLE);
				}

				RenderFileOrDirectoryMenuItems(node);

				ImGui::EndMenu();
			}

			ImGui::TreePop();
        }
    }

    void AssetManagerWindow::RenderAssetItem(AssetTreeNode* node, AssetHandle handle)
    {
        FLARE_PROFILE_FUNCTION();
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
            | ImGuiTreeNodeFlags_FramePadding
            | ImGuiTreeNodeFlags_SpanFullWidth;

        const auto& editorSelection = EditorLayer::GetInstance().Selection;
        if (editorSelection.GetType() == EditorSelectionType::Asset && handle == editorSelection.GetAssetHandle())
            flags |= ImGuiTreeNodeFlags_Selected;

        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(handle));

        const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);
        if (metadata->SubAssets.size() == 0)
            flags |= ImGuiTreeNodeFlags_Leaf;

        bool opened = ImGui::TreeNodeEx(metadata->Name.c_str(), flags, metadata->Name.c_str());

        // Drag and drop

        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload(ASSET_PAYLOAD_NAME, &handle, sizeof(AssetHandle));
            ImGui::EndDragDropSource();
        }

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            OnOpenFile(handle);

        if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            EditorLayer::GetInstance().Selection.SetAsset(handle);
            OnAssetSelectionChanged.Invoke(handle);
        }

        // Context menu

        if (ImGui::BeginPopupContextItem(metadata->Name.c_str()))
        {
            ImGui::Text("Handle: %llu", (uint64_t)handle);

            if (ImGui::MenuItem("Copy Handle"))
            {
                std::string text = std::to_string((uint64_t)handle);
                ImGui::SetClipboardText(text.c_str());
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Open"))
                OnOpenFile(handle);

            if (ImGui::MenuItem("Remove"))
            {
                if (node)
                {
                    node->Handle = NULL_ASSET_HANDLE;
                }

                m_AssetManager->RemoveFromRegistry(handle);
            }

            if (ImGui::MenuItem("Reload"))
            {
                if (AssetManager::IsAssetLoaded(handle))
                    m_AssetManager->ReloadAsset(handle);
            }

            if (node)
                RenderFileOrDirectoryMenuItems(*node);

            ImGui::EndMenu();
        }

        if (opened)
        {
            for (AssetHandle handle : metadata->SubAssets)
                RenderAssetItem(nullptr, handle);

            ImGui::TreePop();
        }
    }

    void AssetManagerWindow::BuildDirectory(uint32_t parentIndex, const std::filesystem::path& path)
    {
        FLARE_PROFILE_FUNCTION();
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
                node.Handle = handle.value_or(NULL_ASSET_HANDLE);
            }

            m_AssetTree[parentIndex].LastChildIndex = (uint32_t)(m_AssetTree.size() - 1);
            m_AssetTree[parentIndex].ChildrenCount++;
        }
    }

    void AssetManagerWindow::OnOpenFile(AssetHandle handle)
    {
        FLARE_PROFILE_FUNCTION();
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

    void AssetManagerWindow::ShowCreateNewFilePopup(const FileNameCallback& callback)
    {
        FLARE_PROFILE_FUNCTION();
        std::memset(m_TextInputBuffer, 0, sizeof(m_TextInputBuffer));

        m_ShowNewFilePopup = true;
        m_OnNewFileNameCallback = callback;
    }

    void AssetManagerWindow::RenderCreateNewFilePopup()
    {
        FLARE_PROFILE_FUNCTION();
        bool createNewFile = false;

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (m_ShowNewFilePopup)
        {
            ImGui::OpenPopup(m_FileNamePopupId);
            m_ShowNewFilePopup = false;
        }

        const auto& style = ImGui::GetStyle();

        const float popupWidth = 300.0f;
        const float popupContentWidth = popupWidth - style.WindowPadding.x * 2.0f;
        ImGui::SetNextWindowSize(ImVec2(popupWidth, 100.0f));
        if (ImGui::BeginPopupModal(m_FileNamePopupId, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::PushItemWidth(popupContentWidth);

            ImGui::PushID("FileNameField");
            ImGui::InputText("", m_TextInputBuffer, 512);
            ImGui::PopID();

            ImGui::PopItemWidth();

            if (ImGui::Button("Create"))
                createNewFile = true;

            ImGui::SameLine();

            if (ImGui::IsKeyPressed(ImGuiKey_Enter))
                createNewFile = true;
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                ImGui::CloseCurrentPopup();

            if (ImGui::Button("Cancel"))
                ImGui::CloseCurrentPopup();

            if (createNewFile)
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        if (createNewFile)
        {
            std::string_view fileName = m_TextInputBuffer;

            if (m_OnNewFileNameCallback)
            {
                m_OnNewFileNameCallback(fileName);
                m_OnNewFileNameCallback = nullptr;
            }
        }
    }

    void AssetManagerWindow::RenderFileOrDirectoryMenuItems(const AssetTreeNode& node)
    {
        ImGui::Separator();

        if (ImGui::MenuItem("Show In File Explorer"))
            Platform::OpenFileExplorer(node.Path);
    }

    void AssetManagerWindow::RenderCreateAssetMenuItems(const AssetTreeNode& rootNode)
    {
        FLARE_PROFILE_FUNCTION();
        if (ImGui::MenuItem("Prefab"))
        {
            ShowCreateNewFilePopup([this, nodeIndex = m_NodeRenderIndex](std::string_view name)
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
            });
        }

        if (ImGui::MenuItem("Sprite"))
        {
            ShowCreateNewFilePopup([this, nodeIndex = m_NodeRenderIndex](std::string_view name)
            {
                std::filesystem::path path = m_AssetTree[nodeIndex].Path / name;
                path.replace_extension(".flrsprite");

                Ref<Sprite> sprite = CreateRef<Sprite>();
                Ref<EditorAssetManager> editorAssetManager = EditorAssetManager::GetInstance();

                SpriteImporter::SerializeSprite(sprite, path);
                editorAssetManager->ImportAsset(path, sprite);
            });
        }

        if (ImGui::MenuItem("Material"))
        {
            ShowCreateNewFilePopup([this, nodeIndex = m_NodeRenderIndex](std::string_view name)
            {
                std::filesystem::path path = m_AssetTree[nodeIndex].Path / name;
                path.replace_extension(".flrmat");

                std::optional<AssetHandle> meshShaderHandle = ShaderLibrary::FindShader("Mesh");

                if (meshShaderHandle)
                {
                    Ref<Material> material = Material::Create(meshShaderHandle.value());
                    MaterialImporter::SerializeMaterial(material, path);

                    m_AssetManager->ImportAsset(path, material);
                    RebuildAssetTree();
                }
                else
                {
                    FLARE_CORE_ERROR("Failed to find Mesh shader");
                }
            });
        }
    }
}
