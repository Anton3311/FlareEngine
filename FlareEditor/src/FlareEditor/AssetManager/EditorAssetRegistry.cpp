#include "EditorAssetRegistry.h"

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
            emitter << YAML::Key << "Path" << YAML::Value << assetPath.generic_string();
            emitter << YAML::EndMap;
        }

        emitter << YAML::EndSeq; // Asset Registry
        emitter << YAML::EndMap;

        std::ofstream outputFile(registryPath);
        outputFile << emitter.c_str();
	}

    bool AssetRegistrySerializer::Deserialize(EditorAssetRegistry& registry, const std::filesystem::path& path)
    {
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
            std::filesystem::path path = root / std::filesystem::path(assetNode["Path"].as<std::string>());


            AssetRegistryEntry entry;
            entry.OwnerType = AssetOwner::Project;
            entry.PackageId = {};

            AssetMetadata& metadata = entry.Metadata;
            metadata.Path = path;
            metadata.Handle = handle;
            metadata.Type = AssetTypeFromString(assetNode["Type"].as<std::string>());

            registry.emplace(handle, entry);
        }

        return true;
    }
}
