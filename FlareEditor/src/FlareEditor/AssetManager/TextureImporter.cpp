#include "TextureImporter.h"

#include "FlareEditor/Serialization/Serialization.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Flare
{
	std::filesystem::path TextureImporter::GetImportSettingsPath(AssetHandle handle)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(handle));

		std::filesystem::path importSettingsPath = AssetManager::GetAssetMetadata(handle)->Path;
		importSettingsPath.replace_extension("flr");

		return importSettingsPath;
	}

	bool TextureImporter::SerialiazeImportSettings(AssetHandle assetHandle, const TextureImportSettings& settings)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(assetHandle));
		std::filesystem::path importSettingsPath = GetImportSettingsPath(assetHandle);
		
		YAML::Emitter emitter;
		emitter << YAML::BeginMap;
		emitter << YAML::Key << "Filtering" << YAML::Value << TextureFilteringToString(settings.Filtering);
		emitter << YAML::Key << "Wrap" << YAML::Value << TextureWrapToString(settings.WrapMode);
		emitter << YAML::EndMap;

		std::ofstream output(importSettingsPath);
		if (!output.is_open())
		{
			FLARE_CORE_ERROR("Failed to write texture import settings file: {0}", importSettingsPath.string());
			return false;
		}

		output << emitter.c_str();
		output.close();

		return true;
	}

	bool TextureImporter::DeserializeImportSettings(AssetHandle assetHandle, TextureImportSettings& settings)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(assetHandle));
		std::filesystem::path importSettingsPath = GetImportSettingsPath(assetHandle);

		if (!std::filesystem::exists(importSettingsPath))
			return false;

		try
		{
			YAML::Node node = YAML::LoadFile(importSettingsPath.generic_string());
			if (YAML::Node filtering = node["Filtering"])
				settings.Filtering = TextureFilteringFromString(filtering.as<std::string>()).value_or(TextureFiltering::Linear);
			else
				settings.Filtering = TextureFiltering::Linear;

			if (YAML::Node wrap = node["Wrap"])
				settings.WrapMode = TextureWrapFromString(wrap.as<std::string>()).value_or(TextureWrap::Repeat);
			else
				settings.WrapMode = TextureWrap::Repeat;
		}
		catch (std::exception& e)
		{
			FLARE_CORE_ERROR("Failed to deserialize texture import settings: {0}", e.what());
			return false;
		}

		return true;
	}

	Ref<Asset> TextureImporter::ImportTexture(const AssetMetadata& metadata)
	{
		std::filesystem::path importSettingsPath = GetImportSettingsPath(metadata.Handle);

		TextureImportSettings importSettings;
		
		if (std::filesystem::exists(importSettingsPath))
			DeserializeImportSettings(metadata.Handle, importSettings);

		TextureSpecifications specifications;
		specifications.Filtering = importSettings.Filtering;
		specifications.Wrap = importSettings.WrapMode;

		return Texture::Create(metadata.Path, specifications);
	}
}