#include "EditorAssetRegistry.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Project/Project.h"
#include "Flare/Serialization/Serialization.h"

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
    const std::filesystem::path EditorAssetRegistry::RegistryFileName = "AssetRegistry.yaml";
    const std::filesystem::path EditorAssetRegistry::AssetsDirectoryName = "Assets";

    void EditorAssetRegistry::Clear()
    {
        m_Entries.clear();
    }

    bool EditorAssetRegistry::Remove(AssetHandle handle)
    {
        FLARE_PROFILE_FUNCTION();

        auto it = m_Entries.find(handle);
        if (it != m_Entries.end())
        {
            const AssetMetadata& metadata = it->second.Metadata;
            for (AssetHandle subAssetHandle : metadata.SubAssets)
            {
                Remove(subAssetHandle);
            }

            m_Entries.erase(handle);

            m_IsDirty = true;

            return true;
        }

        return false;
    }

    void EditorAssetRegistry::Serialize(const std::filesystem::path& path)
    {
        FLARE_PROFILE_FUNCTION();

        if (!m_IsDirty)
            return;

		m_IsDirty = false;

        std::filesystem::path registryPath = path / RegistryFileName;
        std::filesystem::path root = path / AssetsDirectoryName;

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "AssetRegistry" << YAML::Value << YAML::BeginSeq; // Asset Registry

        for (const auto& [handle, entry] : m_Entries)
        {
            std::filesystem::path assetPath = std::filesystem::relative(entry.Metadata.Path, root);

            emitter << YAML::BeginMap;
            emitter << YAML::Key << "Handle" << YAML::Value << handle;
            emitter << YAML::Key << "Type" << YAML::Value << AssetTypeToString(entry.Metadata.Type);
            emitter << YAML::Key << "BuiltIn" << YAML::Value << entry.IsBuiltIn;

            if (entry.Metadata.Source == AssetSource::Memory)
                emitter << YAML::Key << "Name" << YAML::Value << entry.Metadata.Name;
            else
                emitter << YAML::Key << "Path" << YAML::Value << assetPath.generic_string();

            emitter << YAML::Key << "Source" << YAML::Value << AssetSourceToString(entry.Metadata.Source);
            emitter << YAML::Key << "Parent" << YAML::Value << entry.Metadata.Parent;
            emitter << YAML::Key << "SubAssets" << YAML::Value << YAML::BeginSeq; // Sub assets

            for (AssetHandle subAsset : entry.Metadata.SubAssets)
                emitter << YAML::Value << subAsset;

            emitter << YAML::EndSeq;

            emitter << YAML::EndMap;
        }

        emitter << YAML::EndSeq; // Asset Registry
        emitter << YAML::EndMap;

        {
            FLARE_PROFILE_SCOPE("WriteTextFile");

			std::ofstream outputFile(registryPath);
			outputFile << emitter.c_str();
        }
    }

    bool EditorAssetRegistry::Deserialize(const std::filesystem::path& path)
    {
        FLARE_PROFILE_FUNCTION();

        std::filesystem::path registryPath = path / RegistryFileName;
        std::filesystem::path root = path / AssetsDirectoryName;

        std::ifstream inputFile(registryPath);
        if (!inputFile)
        {
            FLARE_CORE_ERROR("Failed to read asset registry file: {0}", registryPath.string());
            return false;
        }

        YAML::Node node = YAML::Load(inputFile);
        YAML::Node registryNode = node["AssetRegistry"];

        if (!registryNode)
            return false;

        for (auto assetNode : registryNode)
        {
            AssetHandle handle = assetNode["Handle"].as<AssetHandle>();
            
            AssetRegistryEntry entry{};
            AssetMetadata& metadata = entry.Metadata;
            metadata.Handle = handle;
            metadata.Source = AssetSource::File;
            metadata.Type = AssetTypeFromString(assetNode["Type"].as<std::string>());

            if (YAML::Node source = assetNode["Source"])
                metadata.Source = AssetSourceFromString(source.as<std::string>());

            if (YAML::Node builtIn = assetNode["BuiltIn"])
                entry.IsBuiltIn = builtIn.as<bool>();

            if (metadata.Source == AssetSource::Memory)
            {
                if (YAML::Node name = assetNode["Name"])
                    metadata.Name = name.as<std::string>();
            }
            else
            {
                if (YAML::Node pathNode = assetNode["Path"])
                    metadata.Path = root / std::filesystem::path(pathNode.as<std::string>());

                metadata.Name = metadata.Path.filename().generic_string();
            }

            if (YAML::Node parent = assetNode["Parent"])
                metadata.Parent = parent.as<AssetHandle>();
            else
                metadata.Parent = NULL_ASSET_HANDLE;

            if (YAML::Node subAssets = assetNode["SubAssets"])
            {
                for (YAML::Node subAsset : subAssets)
                    metadata.SubAssets.push_back(subAsset.as<AssetHandle>());
            }

			m_Entries.emplace(handle, entry);
        }

        return true;
    }
}
