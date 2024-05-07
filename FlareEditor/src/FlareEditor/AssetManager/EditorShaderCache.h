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

		Ref<const ShaderMetadata> FindShaderMetadata(AssetHandle shaderHandle) override;
		Ref<const ComputeShaderMetadata> FindComputeShaderMetadata(AssetHandle shaderHandle) override;

		bool HasCache(AssetHandle shaderHandle,
			ShaderTargetEnvironment targetEnvironment,
			ShaderStageType stage) override;

		void SetShaderEntry(AssetHandle shaderHandle, Ref<const ShaderMetadata> metadata);
		void SetComputeShaderEntry(AssetHandle shaderHandle, Ref<const ComputeShaderMetadata> metadata);

		std::filesystem::path GetCacheDirectoryPath(AssetHandle shaderHandle);
		std::string GetCacheFileName(std::string_view shaderName, ShaderStageType stageType);
	public:
		static EditorShaderCache& GetInstance();
	private:
		std::unordered_map<AssetHandle, Ref<const ShaderMetadata>> m_Entries;
		std::unordered_map<AssetHandle, Ref<const ComputeShaderMetadata>> m_ComputeShaderEntries;
	};
}