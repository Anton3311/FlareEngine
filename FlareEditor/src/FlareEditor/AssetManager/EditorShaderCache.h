#pragma once

#include "Flare/Renderer/ShaderCacheManager.h"

#include <filesystem>

namespace Flare
{
	class EditorShaderCache : public ShaderCacheManager
	{
	public:
		void SetCache(AssetHandle shaderHandle, ShaderStageType stageType, const std::vector<uint32_t>& compiledShader) override;

		std::filesystem::path GetCacheFilePath(AssetHandle shaderHandle, ShaderStageType stageType) const;
		std::optional<std::vector<uint32_t>> FindCache(AssetHandle shaderHandle, ShaderStageType stageType) override;

		Ref<const GraphicsShaderMetadata> FindShaderMetadata(AssetHandle shaderHandle) override;
		Ref<const ComputeShaderMetadata> FindComputeShaderMetadata(AssetHandle shaderHandle) override;

		bool HasCache(AssetHandle shaderHandle, ShaderStageType stage) override;

		void SetShaderEntry(AssetHandle shaderHandle, Ref<const GraphicsShaderMetadata> metadata);
		void SetComputeShaderEntry(AssetHandle shaderHandle, Ref<const ComputeShaderMetadata> metadata);

		std::filesystem::path GetCacheDirectoryPath() const;
		std::string GetCacheFileName(AssetHandle shaderHandle, ShaderStageType stageType) const;
	public:
		static EditorShaderCache& GetInstance();
	private:
		std::unordered_map<AssetHandle, Ref<const GraphicsShaderMetadata>> m_Entries;
		std::unordered_map<AssetHandle, Ref<const ComputeShaderMetadata>> m_ComputeShaderEntries;
	};
}