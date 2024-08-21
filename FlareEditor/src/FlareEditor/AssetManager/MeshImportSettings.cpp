#include "MeshImportSettings.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Serialization/Serialization.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Flare
{
	std::filesystem::path MeshImportSettingsSerializer::GetImportSettingsPath(AssetHandle handle)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(handle));

		const AssetMetadata* metadata = AssetManager::GetAssetMetadata(handle);

		FLARE_CORE_ASSERT(metadata);
		FLARE_CORE_ASSERT(metadata->Type == AssetType::Mesh);

		return std::filesystem::path(metadata->Path).replace_extension("flr");
	}

	void MeshImportSettingsSerializer::Serialize(AssetHandle handle, const MeshImportSettings& settings)
	{
		FLARE_PROFILE_FUNCTION();

		std::filesystem::path importSettingsPath = GetImportSettingsPath(handle);

		YAML::Emitter emitter;

		emitter << YAML::BeginMap;

		emitter << YAML::Key << "ImportMaterials" << settings.ImportMaterials;
		emitter << YAML::Key << "DefaultMaterial" << settings.DefaultMaterial;
		emitter << YAML::Key << "PreserveHierarchy" << settings.PreserveHierarchy;
		emitter << YAML::Key << "GeneratePrefab" << settings.GeneratePrefab;
		emitter << YAML::Key << "GeneratedPrefabHandle" << settings.GeneratedPrefabHandle;

		emitter << YAML::EndMap;

		std::ofstream output(importSettingsPath);
		output << emitter.c_str();
		output.close();
	}

	bool MeshImportSettingsSerializer::Deserialize(AssetHandle handle, MeshImportSettings& outSettings)
	{
		FLARE_PROFILE_FUNCTION();

		std::filesystem::path path = GetImportSettingsPath(handle);

		if (!std::filesystem::exists(path))
			return false;

		try
		{
			YAML::Node root = YAML::LoadFile(path.generic_string());

			if (YAML::Node importMaterials = root["ImportMaterials"])
				outSettings.ImportMaterials = importMaterials.as<bool>();
			if (YAML::Node defaultMaterial = root["DefaultMaterial"])
				outSettings.DefaultMaterial = defaultMaterial.as<AssetHandle>();
			if (YAML::Node node= root["PreserveHierachy"])
				outSettings.PreserveHierarchy = node.as<bool>();
			if (YAML::Node node= root["GeneratePrefab"])
				outSettings.GeneratePrefab = node.as<bool>();
			if (YAML::Node node= root["GeneratedPrefabHandle"])
				outSettings.GeneratedPrefabHandle = node.as<AssetHandle>();

			return true;
		}
		catch (std::exception& exception)
		{
			FLARE_CORE_ERROR("Failed to deserialize MeshImportSettings: {}", exception.what());
		}

		return false;
	}
}
