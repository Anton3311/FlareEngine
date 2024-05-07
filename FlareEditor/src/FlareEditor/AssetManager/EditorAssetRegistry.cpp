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
    const std::filesystem::path AssetRegistrySerializer::RegistryFileName = "AssetRegistry.yaml";
    const std::filesystem::path AssetRegistrySerializer::AssetsDirectoryName = "Assets";

	void Flare::AssetRegistrySerializer::Serialize(const EditorAssetRegistry& registry, const std::filesystem::path& path)
	{
        FLARE_PROFILE_FUNCTION();

        std::filesystem::path registryPath = path / RegistryFileName;
        std::filesystem::path root = path / AssetsDirectoryName;

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "AssetRegistry" << YAML::Value << YAML::BeginSeq; // Asset Registry

        for (const auto& [handle, entry] : registry)
        {
            if (entry.OwnerType != AssetOwner::Project)
                continue;

            std::filesystem::path assetPath = std::filesystem::relative(entry.Metadata.Path, root);

            emitter << YAML::BeginMap;
            emitter << YAML::Key << "Handle" << YAML::Value << handle;
            emitter << YAML::Key << "Type" << YAML::Value << AssetTypeToString(entry.Metadata.Type);

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

        std::ofstream outputFile(registryPath);
        outputFile << emitter.c_str();
	}

    bool AssetRegistrySerializer::Deserialize(EditorAssetRegistry& registry, const std::filesystem::path& path, std::optional<UUID> packageId)
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
            
            AssetRegistryEntry entry;
            entry.OwnerType = packageId.has_value() ? AssetOwner::Package : AssetOwner::Project;
            entry.PackageId = packageId;

            AssetMetadata& metadata = entry.Metadata;
            metadata.Handle = handle;
            metadata.Source = AssetSource::File;
            metadata.Type = AssetTypeFromString(assetNode["Type"].as<std::string>());

            if (YAML::Node source = assetNode["Source"])
                metadata.Source = AssetSourceFromString(source.as<std::string>());

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

            registry.emplace(handle, entry);
        }

        return true;
    }
}
