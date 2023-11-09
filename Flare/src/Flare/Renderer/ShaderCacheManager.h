#pragma once

#include "FlareCore/Core.h"

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/RendererAPI.h"

#include "Flare/Renderer/Shader.h"

#include <vector>
#include <optional>

namespace Flare
{
	class FLARE_API ShaderCacheManager
	{
	public:
		virtual void SetCache(AssetHandle shaderHandle,
			ShaderTargetEnvironment targetEnvironment,
			ShaderStageType stageType,
			const std::vector<uint32_t>& compiledShader) = 0;

		virtual std::optional<std::vector<uint32_t>> FindCache(AssetHandle shaderHandle,
			ShaderTargetEnvironment targetEnvironment,
			ShaderStageType stageType) = 0;

		virtual std::optional<ShaderFeatures> FindShaderFeatures(AssetHandle shaderHandle) = 0;

		virtual bool HasCache(AssetHandle shaderHandle,
			ShaderTargetEnvironment targetEnvironment,
			ShaderStageType stage) = 0;
	public:
		static void Uninitialize();
		static void SetInstance(Scope<ShaderCacheManager>&& cacheManager);
		static const Scope<ShaderCacheManager>& GetInstance();
	};
}