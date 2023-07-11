#include "EditorAssetManager.h"

#include "Flare/Core/Assert.h"
#include "Flare/Core/Log.h"

#include "Flare/AssetManager/Importers/TextureImporter.h"
#include "Flare/AssetManager/Importers/SceneImporter.h"

#include "Flare/Serialization/Serialization.h"
#include "Flare/Project/Project.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <string>

namespace YAML
{
    Emitter& operator<<(Emitter& emitter, std::string_view string)
    {
        emitter << std::string(string.data(), string.size());
        return emitter;
    }
}

namespace Flare
{
    std::filesystem::path EditorAssetManager::s_RegistryFileName = "AssetRegistry.yaml";

    EditorAssetManager::EditorAssetManager(const std::filesystem::path& root)
        : m_Root(root)
    {
        if (!std::filesystem::exists(root))
            std::filesystem::create_directories(root);

        m_AssetImporters.emplace(AssetType::Texture, TextureImporter::ImportTexture);
        m_AssetImporters.emplace(AssetType::Scene, SceneImporter::ImportScene);

        DeserializeRegistry();
    }

    Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
    {
        auto assetIterator = m_LoadedAssets.find(handle);
        if (assetIterator != m_LoadedAssets.end())
            return assetIterator->second;

        auto registryIterator = m_Registry.find(handle);
        if (registryIterator == m_Registry.end())
            return nullptr;

        return LoadAsset(registryIterator->second);
    }

    const AssetMetadata* EditorAssetManager::GetAssetMetadata(AssetHandle handle)
    {
        auto it = m_Registry.find(handle);
        if (it == m_Registry.end())
            return nullptr;
        return &it->second;
    }

    bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle)
    {
        return m_Registry.find(handle) != m_Registry.end();
    }

    bool EditorAssetManager::IsAssetLoaded(AssetHandle handle)
    {
        return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
    }

    std::optional<AssetHandle> EditorAssetManager::FindAssetByPath(const std::filesystem::path& path)
    {
        auto it = m_FilepathToAssetHandle.find(path);
        if (it == m_FilepathToAssetHandle.end())
            return {};
        return it->second;
    }

    AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& path)
    {
        AssetType type = AssetType::None;
        std::filesystem::path extension = path.extension();

        if (extension == ".png")
            type = AssetType::Texture;
        else if (extension == ".flare")
            type = AssetType::Scene;

        AssetHandle handle;
        AssetMetadata metadata;
        metadata.Path = path;
        metadata.Type = type;
        metadata.Handle = handle;

        m_Registry.emplace(handle, metadata);
        m_FilepathToAssetHandle.emplace(path, handle);

        SerializeRegistry();

        return handle;
    }

    void EditorAssetManager::UnloadAsset(AssetHandle handle)
    {
        auto it = m_LoadedAssets.find(handle);
        if (it == m_LoadedAssets.end())
            return;

        m_LoadedAssets.erase(it);
    }

    void EditorAssetManager::RemoveFromRegistry(AssetHandle handle)
    {
        auto it = m_Registry.find(handle);
        if (it != m_Registry.end())
        {
            UnloadAsset(handle);
            m_Registry.erase(handle);
            SerializeRegistry();
        }
    }

    Ref<Asset> EditorAssetManager::LoadAsset(const AssetMetadata& metadata)
    {
        auto importerIterator = m_AssetImporters.find(metadata.Type);
        if (importerIterator == m_AssetImporters.end())
        {
            FLARE_CORE_ASSERT("Cannot import '{0}', because an imported for that asset type is not provided");
            return nullptr;
        }

        Ref<Asset> asset = importerIterator->second(metadata);
        asset->Handle = metadata.Handle;
        m_LoadedAssets.emplace(metadata.Handle, asset);
        return asset;
    }

    void EditorAssetManager::SerializeRegistry()
    {
        std::filesystem::path path = Project::GetActive()->Location / s_RegistryFileName;

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "AssetRegistry" << YAML::Value << YAML::BeginSeq; // Asset Registry

        for (const auto& [handle, metadata] : m_Registry)
        {
            std::filesystem::path assetPath = std::filesystem::relative(metadata.Path, m_Root);

            emitter << YAML::BeginMap;
            emitter << YAML::Key << "Handle" << YAML::Value << handle;
            emitter << YAML::Key << "Type" << YAML::Value << AssetTypeToString(metadata.Type);
            emitter << YAML::Key << "Path" << YAML::Value << assetPath.generic_string();
            emitter << YAML::EndMap;
        }

        emitter << YAML::EndSeq; // Asset Registry
        emitter << YAML::EndMap;

        std::ofstream outputFile(path);
        outputFile << emitter.c_str();
    }

    void EditorAssetManager::DeserializeRegistry()
    {
        std::filesystem::path registryPath = Project::GetActive()->Location / s_RegistryFileName;

        std::ifstream inputFile(registryPath);
        if (!inputFile)
        {
            FLARE_CORE_ERROR("Failed to read scene file: {0}", registryPath.string());
            return;
        }

        YAML::Node node = YAML::Load(inputFile);

        YAML::Node registry = node["AssetRegistry"];
        if (!registry)
            return;

        for (auto assetNode : registry)
        {
            AssetHandle handle = assetNode["Handle"].as<AssetHandle>();
            std::filesystem::path path = m_Root / std::filesystem::path(assetNode["Path"].as<std::string>());

            AssetMetadata metadata;
            metadata.Path = path;
            metadata.Handle = handle;
            metadata.Type = AssetTypeFromString(assetNode["Type"].as<std::string>());

            m_Registry.emplace(handle, metadata);
            m_FilepathToAssetHandle.emplace(path, handle);
        }
    }
}
