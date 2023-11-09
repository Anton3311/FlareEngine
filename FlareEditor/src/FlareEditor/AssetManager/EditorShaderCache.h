#pragma once

#include "Flare/Renderer/ShaderCacheManager.h"

#include <filesystem>

namespace Flare
{
	class EditorShaderCache : public ShaderCacheManager
	{
	public:
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

		void SetShaderFeatures(AssetHandle shaderHandle, ShaderFeatures features);

		std::filesystem::path GetCacheDirectoryPath(AssetHandle shaderHandle);
		std::string GetCacheFileName(std::string_view shaderName, ShaderTargetEnvironment targetEnvironemt, ShaderStageType stageType);
	private:
		std::unordered_map<AssetHandle, ShaderFeatures> m_ShaderFeatures;
	};
}