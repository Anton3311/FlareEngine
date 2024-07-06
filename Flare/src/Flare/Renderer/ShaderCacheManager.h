#pragma once

#include "FlareCore/Core.h"

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/ShaderMetadata.h"

#include <vector>
#include <optional>

namespace Flare
{
	struct ShaderMetadata;
	struct ComputeShaderMetadata;

	class FLARE_API ShaderCacheManager
	{
	public:
		virtual void SetCache(AssetHandle shaderHandle, ShaderStageType stageType, const std::vector<uint32_t>& compiledShader) = 0;

		virtual std::optional<std::vector<uint32_t>> FindCache(AssetHandle shaderHandle, ShaderStageType stageType) = 0;

		virtual Ref<const ShaderMetadata> FindShaderMetadata(AssetHandle shaderHandle) = 0;
		virtual Ref<const ComputeShaderMetadata> FindComputeShaderMetadata(AssetHandle shaderHandle) = 0;

		virtual bool HasCache(AssetHandle shaderHandle, ShaderStageType stage) = 0;
	public:
		static void Uninitialize();
		static void SetInstance(Scope<ShaderCacheManager>&& cacheManager);
		static const Scope<ShaderCacheManager>& GetInstance();
	};
}