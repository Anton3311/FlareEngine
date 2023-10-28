#include "AssetsPackage.h"

#include "FlareCore/Assert.h"
#include "Flare/Serialization/Serialization.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Flare
{
    const std::filesystem::path AssetsPackage::InernalPackagesLocation = "../Packages";
    const std::filesystem::path PackageDependenciesSerializer::PackagesFileName = "Packages.yaml";

	void PackageDependenciesSerializer::Serialize(const std::map<UUID, AssetsPackage>& packages, const std::filesystem::path& path)
	{
        std::filesystem::path filePath = path / PackagesFileName;

        YAML::Emitter emitter;
        emitter << YAML::BeginMap;
        emitter << YAML::Key << "Packages" << YAML::Value << YAML::BeginSeq; // Asset Registry

        for (const auto& [id, package] : packages)
        {
            emitter << YAML::BeginMap;
            emitter << YAML::Key << "Id" << YAML::Value << package.Id;
            emitter << YAML::Key << "Type" << YAML::Value << (package.PackageType == AssetsPackage::Type::Internal ? "Internal" : "");
            emitter << YAML::Key << "Name" << YAML::Value << package.Name;
            emitter << YAML::EndMap;
        }

        emitter << YAML::EndSeq; // Asset Registry
        emitter << YAML::EndMap;

        std::ofstream outputFile(filePath);
        outputFile << emitter.c_str();
	}

	bool PackageDependenciesSerializer::Deserialize(std::map<UUID, AssetsPackage>& packages, const std::filesystem::path& path)
	{
        std::filesystem::path filePath = path / PackagesFileName;

        std::ifstream inputFile(filePath);
        if (!inputFile)
        {
            FLARE_CORE_ERROR("Failed to read packages file: {0}", filePath.string());
            return false;
        }

        YAML::Node node = YAML::Load(inputFile);
        YAML::Node registryNode = node["Packages"];

        if (!registryNode)
            return false;

        for (auto packageNode : registryNode)
        {
            AssetsPackage package;

            if (YAML::Node idNode = packageNode["Id"])
                package.Id = idNode.as<UUID>();
            if (YAML::Node idNode = packageNode["Name"])
                package.Name = idNode.as<std::string>();
            if (YAML::Node idNode = packageNode["Type"])
                package.PackageType = AssetsPackage::Type::Internal;

            packages.emplace(package.Id, std::move(package));
        }

        return true;
	}

    bool PackageDependenciesSerializer::DeserializePackage(AssetsPackage& package, const std::filesystem::path& path)
    {
        std::ifstream inputFile(path);
        if (!inputFile)
        {
            FLARE_CORE_ERROR("Failed to read package file: {0}", path.string());
            return false;
        }

        YAML::Node root = YAML::Load(inputFile);

        if (!root)
            return false;

        if (YAML::Node nameNode = root["Name"])
            package.Name = nameNode.as<std::string>();

        return true;
    }
}
