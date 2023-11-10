#pragma once

#include "Flare/Renderer/ShaderCacheManager.h"
#include "FlareCore/Serialization/Serialization.h"

#include <filesystem>

namespace Flare
{
	class EditorShaderCache : public ShaderCacheManager
	{
	public:
		struct ShaderEntry
		{
			ShaderEntry(ShaderFeatures features, SerializableObjectDescriptor&& descriptor)
				: Features(features), SerializationDescriptor(std::move(descriptor)) {}

			ShaderFeatures Features;
			SerializableObjectDescriptor SerializationDescriptor;
		};

		void SetCache(AssetHandle shaderHandle,
			ShaderTargetEnvironment targetEnvironment,
			ShaderStageType stageType,
			const std::vector<uint32_t>& compiledShader) override;

		std::optional<std::vector<uint32_t>> FindCache(AssetHandle shaderHandle,
			ShaderTargetEnvironment targetEnvironment,
			ShaderStageType stageType) override;

		std::optional<ShaderFeatures> FindShaderFeatures(AssetHandle shaderHandle) override;

		bool HasCache(AssetHandle shaderHandle,
			ShaderTargetEnvironment targetEnvironment,
			ShaderStageType stage) override;

		std::optional<const ShaderEntry*> GetShaderEntry(AssetHandle shaderHandle) const;
		void SetShaderEntry(AssetHandle shaderHandle,
			ShaderFeatures features,
			SerializableObjectDescriptor&& serializationDescriptor);

		std::filesystem::path GetCacheDirectoryPath(AssetHandle shaderHandle);
		std::string GetCacheFileName(std::string_view shaderName, ShaderTargetEnvironment targetEnvironemt, ShaderStageType stageType);
	private:
		std::unordered_map<AssetHandle, ShaderEntry> m_Entries;
	};
}