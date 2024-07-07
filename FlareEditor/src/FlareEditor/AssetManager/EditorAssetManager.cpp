#include "EditorAssetManager.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Serialization/Serialization.h"
#include "Flare/Project/Project.h"

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/Font.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "FlareEditor/EditorLayer.h"

#include "FlareEditor/ShaderCompiler/ShaderCompiler.h"

#include "FlareEditor/AssetManager/TextureImporter.h"
#include "FlareEditor/AssetManager/SpriteImporter.h"
#include "FlareEditor/AssetManager/PrefabImporter.h"
#include "FlareEditor/Serialization/SceneSerializer.h"
#include "FlareEditor/AssetManager/MaterialImporter.h"
#include "FlareEditor/AssetManager/MeshImporter.h"
#include "FlareEditor/AssetManager/ShaderImporter.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <string>

namespace Flare
{
    EditorAssetManager::EditorAssetManager()
    {
        m_AssetImporters.emplace(AssetType::Texture, TextureImporter::ImportTexture);
        m_AssetImporters.emplace(AssetType::Prefab, PrefabImporter::ImportPrefab);
        m_AssetImporters.emplace(AssetType::Material, MaterialImporter::ImportMaterial);
        m_AssetImporters.emplace(AssetType::Mesh, MeshImporter::ImportMesh);
        m_AssetImporters.emplace(AssetType::Sprite, SpriteImporter::ImportSprite);
        m_AssetImporters.emplace(AssetType::Scene, [](const AssetMetadata& metadata) -> Ref<Asset>
        {
			FLARE_PROFILE_FUNCTION();
            Ref<Scene> scene = CreateRef<Scene>(EditorLayer::GetInstance().GetECSContext());
            SceneSerializer::Deserialize(scene, metadata.Path,
                EditorLayer::GetInstance().GetCamera(),
                EditorLayer::GetInstance().GetSceneViewSettings());

            return scene;
        });

        m_AssetImporters.emplace(AssetType::Shader, ShaderImporter::ImportShader);
        m_AssetImporters.emplace(AssetType::ComputeShader, ShaderImporter::ImportComputeShader);
        m_AssetImporters.emplace(AssetType::Font, [](const AssetMetadata& metadata) -> Ref<Asset>
        {
			FLARE_PROFILE_FUNCTION();
            return CreateRef<Font>(metadata.Path);
        });
    }

    void EditorAssetManager::Reinitialize()
    {
        FLARE_PROFILE_FUNCTION();
        m_LoadedAssets.clear();
        m_Registry.Clear();
        m_FilepathToAssetHandle.clear();

        ShaderLibrary::Clear();

        FLARE_CORE_ASSERT(Project::GetActive());

        std::filesystem::path root = GetAssetsRoot();
        if (!std::filesystem::exists(root))
            std::filesystem::create_directories(root);

        DeserializeRegistry();
        ImportBuiltinAssets();

        for (const auto& entry : m_Registry.GetEntries())
        {
            bool isShaderAsset = entry.second.Metadata.Type == AssetType::Shader || entry.second.Metadata.Type == AssetType::ComputeShader;
            if (isShaderAsset && entry.second.Metadata.Source == AssetSource::File)
            {
                std::string shaderFileName = entry.second.Metadata.Path.filename().string();
                size_t dotPosition = shaderFileName.find_first_of(".");

                std::string_view shaderName = shaderFileName;
                if (dotPosition != std::string_view::npos)
                    shaderName = std::string_view(shaderFileName).substr(0, dotPosition);

                ShaderLibrary::AddShader(shaderName, entry.second.Metadata.Handle);
            }
        }
    }

    Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
    {
        auto assetIterator = m_LoadedAssets.find(handle);
        if (assetIterator != m_LoadedAssets.end())
            return assetIterator->second;

        if (const AssetRegistryEntry* entry = m_Registry.FindEntry(handle))
			return LoadAsset(entry->Metadata);

        return nullptr;
    }

    const AssetMetadata* EditorAssetManager::GetAssetMetadata(AssetHandle handle)
    {
        if (const AssetRegistryEntry* entry = m_Registry.FindEntry(handle))
            return &entry->Metadata;

        return nullptr;
    }

    bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle)
    {
        return m_Registry.Contains(handle);
    }

    bool EditorAssetManager::IsAssetLoaded(AssetHandle handle)
    {
        return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
    }

    std::filesystem::path EditorAssetManager::GetBuiltInContentPath() const
    {
        return std::filesystem::absolute("../Content/");
    }

    std::optional<AssetHandle> EditorAssetManager::FindAssetByPath(const std::filesystem::path& path)
    {
        FLARE_PROFILE_FUNCTION();
        auto it = m_FilepathToAssetHandle.find(std::filesystem::absolute(path));
        if (it == m_FilepathToAssetHandle.end())
            return {};
        return it->second;
    }

    AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& path, AssetHandle parentAsset)
    {
        FLARE_PROFILE_FUNCTION();

        auto pathIterator = m_FilepathToAssetHandle.find(path);
        if (pathIterator != m_FilepathToAssetHandle.end())
        {
            if (auto* entry = m_Registry.FindEntry(pathIterator->second))
            {
                return entry->Metadata.Handle;
            }
        }

        AssetType type = AssetType::None;
        std::filesystem::path extension = path.extension();

        if (extension == ".png")
            type = AssetType::Texture;
        else if (extension == ".dds")
            type = AssetType::Texture;
        else if (extension == ".jpg")
            type = AssetType::Texture;
        else if (extension == ".jpeg")
            type = AssetType::Texture;
        else if (extension == ".flare")
            type = AssetType::Scene;
        else if (extension == ".flrprefab")
            type = AssetType::Prefab;
        else if (extension == ".flrmat")
            type = AssetType::Material;
        else if (extension == ".flrsprite")
            type = AssetType::Sprite;
        else if (extension == ".glsl")
        {
            std::string_view computeShaderExtension = ".compute.glsl";
            std::string pathString = path.string();
            size_t position = pathString.find(computeShaderExtension);

            if (position == std::string::npos)
            {
                type = AssetType::Shader;
            }
            else
            {
                type = AssetType::ComputeShader;
            }
        }
        else if (extension == ".ttf")
            type = AssetType::Font;
        else if (extension == ".fbx" || extension == ".gltf")
            type = AssetType::Mesh;
        else
            return NULL_ASSET_HANDLE;

        AssetHandle handle;

        AssetRegistryEntry& entry = m_Registry.Insert(handle);
        AssetMetadata& metadata = entry.Metadata;
        metadata.Path = path;
        metadata.Type = type;
        metadata.Handle = handle;
        metadata.Parent = parentAsset;
        metadata.Name = metadata.Path.filename().generic_string();
        metadata.Source = AssetSource::File;

        if (parentAsset == NULL_ASSET_HANDLE)
        {
            m_FilepathToAssetHandle.emplace(std::filesystem::absolute(path), handle);
        }
        else
        {
            if (auto* parentEntry = m_Registry.FindEntry(parentAsset))
            {
                parentEntry->Metadata.SubAssets.push_back(handle);
            }
        }

        return handle;
    }

    AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& path, const Ref<Asset> asset, AssetHandle parentAsset)
    {
        FLARE_PROFILE_FUNCTION();
        AssetHandle handle;

        AssetRegistryEntry& entry = m_Registry.Insert(handle);
        AssetMetadata& metadata = entry.Metadata;
        metadata.Handle = handle;
        metadata.Path = path;
        metadata.Type = asset->GetType();
        metadata.Parent = parentAsset;
        metadata.Source = AssetSource::File;
        metadata.Name = metadata.Path.filename().generic_string();

        asset->Handle = handle;

        if (parentAsset == NULL_ASSET_HANDLE)
        {
            m_FilepathToAssetHandle.emplace(std::filesystem::absolute(path), handle);
        }
        else
        {
            if (auto* parentEntry = m_Registry.FindEntry(parentAsset))
            {
                parentEntry->Metadata.SubAssets.push_back(handle);
            }
        }

        m_LoadedAssets.emplace(asset->Handle, asset);
        return handle;
    }

    AssetHandle EditorAssetManager::ImportMemoryOnlyAsset(std::string_view name, const Ref<Asset> asset, AssetHandle parentAsset)
    {
        FLARE_PROFILE_FUNCTION();
        AssetHandle handle;

        AssetRegistryEntry& entry = m_Registry.Insert(handle);
        AssetMetadata& metadata = entry.Metadata;
        metadata.Handle = handle;
        metadata.Name = name;
        metadata.Type = asset->GetType();
        metadata.Source = AssetSource::Memory;
        metadata.Parent = parentAsset;

        asset->Handle = handle;

		if (auto* parentEntry = m_Registry.FindEntry(parentAsset))
		{
			parentEntry->Metadata.SubAssets.push_back(handle);
		}

        m_LoadedAssets.emplace(asset->Handle, asset);
        return handle;
    }

    void EditorAssetManager::ReloadAsset(AssetHandle handle)
    {
        FLARE_PROFILE_FUNCTION();
        FLARE_CORE_ASSERT(IsAssetHandleValid(handle));

        auto* entry = m_Registry.FindEntry(handle);
        if (entry == nullptr)
            return;

        if (entry->Metadata.Type == AssetType::Shader)
        {
            ShaderCompiler::Compile(handle, true);

            Ref<Shader> shader = As<Shader>(m_LoadedAssets[handle]);
            shader->Load();
            return;
        }

        LoadAsset(entry->Metadata);
    }

    void EditorAssetManager::UnloadAsset(AssetHandle handle)
    {
        FLARE_PROFILE_FUNCTION();
        auto it = m_LoadedAssets.find(handle);
        if (it == m_LoadedAssets.end())
            return;

        m_LoadedAssets.erase(it);
    }

    void EditorAssetManager::ReloadPrefabs()
    {
        FLARE_PROFILE_FUNCTION();
        for (const auto& [handle, asset] : m_Registry.GetEntries())
        {
            if (asset.Metadata.Type == AssetType::Prefab && IsAssetLoaded(handle))
                ReloadAsset(handle);
        }
    }

    void EditorAssetManager::RemoveFromRegistry(AssetHandle handle)
    {
        FLARE_PROFILE_FUNCTION();
        RemoveFromRegistryWithoutSerialization(handle);
        SerializeRegistry();
    }

    void EditorAssetManager::SetLoadedAsset(AssetHandle handle, const Ref<Asset>& asset)
    {
        if (!IsAssetHandleValid(handle))
            return;

        const AssetMetadata* metadata = GetAssetMetadata(handle);
        FLARE_CORE_ASSERT(metadata->Type == asset->GetType());

        m_LoadedAssets[handle] = asset;
        asset->Handle = handle;
    }

    Ref<Asset> EditorAssetManager::LoadAsset(AssetHandle handle)
    {
        FLARE_CORE_ASSERT(IsAssetHandleValid(handle));
        return LoadAsset(*GetAssetMetadata(handle));
    }

    std::filesystem::path EditorAssetManager::GetAssetsRoot()
    {
        FLARE_CORE_ASSERT(Project::GetActive());
        return Project::GetActive()->Location / "Assets";
    }

    void EditorAssetManager::RemoveFromRegistryWithoutSerialization(AssetHandle handle)
    {
        FLARE_PROFILE_FUNCTION();
    }

    void EditorAssetManager::ImportBuiltinAssets()
    {
        FLARE_PROFILE_FUNCTION();

        std::filesystem::path builtingContentPath = GetBuiltInContentPath();

        FLARE_CORE_ASSERT(std::filesystem::exists(builtingContentPath));

        ImportBuiltinAssets(builtingContentPath);
    }

    void EditorAssetManager::ImportBuiltinAssets(const std::filesystem::path& directoryPath)
    {
        FLARE_PROFILE_FUNCTION();
		for (std::filesystem::path child : std::filesystem::directory_iterator(directoryPath))
		{
            if (std::filesystem::is_directory(child))
            {
                ImportBuiltinAssets(child);
            }
            else
            {
				auto it = m_FilepathToAssetHandle.find(child);
				if (it == m_FilepathToAssetHandle.end())
				{
					AssetHandle handle = ImportAsset(child);
                    if (handle == NULL_ASSET_HANDLE)
                        continue;

					auto* entry = m_Registry.FindEntry(handle);
                    FLARE_CORE_ASSERT(entry);
                    entry->IsBuiltIn = true;
				}
			}
		}
    }

    Ref<Asset> EditorAssetManager::LoadAsset(const AssetMetadata& metadata)
    {
        FLARE_PROFILE_FUNCTION();

        auto importerIterator = m_AssetImporters.find(metadata.Type);
        if (importerIterator == m_AssetImporters.end())
        {
            FLARE_CORE_ASSERT("Cannot import '{0}', because an imported for that asset type is not provided");
            return nullptr;
        }

        Ref<Asset> asset = importerIterator->second(metadata);

        if (!asset)
            return nullptr;

        asset->Handle = metadata.Handle;
        m_LoadedAssets[metadata.Handle] = asset;
        return asset;
    }

    void EditorAssetManager::SerializeRegistry()
    {
        FLARE_PROFILE_FUNCTION();
        m_Registry.Serialize(Project::GetActive()->Location);
    }

    void EditorAssetManager::DeserializeRegistry()
    {
        FLARE_PROFILE_FUNCTION();

        {
            FLARE_PROFILE_SCOPE("Clear");
			m_Registry.Clear();
			m_FilepathToAssetHandle.clear();
        }

        m_Registry.Deserialize(Project::GetActive()->Location);

        {
            FLARE_PROFILE_SCOPE("GenerateFilepathToAssetHandleMapping");
            for (const auto& [handle, entry] : m_Registry.GetEntries())
            {
				m_FilepathToAssetHandle.emplace(std::filesystem::absolute(entry.Metadata.Path), handle);
            }
        }
    }
}
