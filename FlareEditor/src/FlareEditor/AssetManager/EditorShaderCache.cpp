#include "EditorShaderCache.h"

#include "FlareCore/Core.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/RendererAPI.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <fstream>

namespace Flare
{
	void EditorShaderCache::SetCache(AssetHandle shaderHandle, ShaderStageType stageType, const std::vector<uint32_t>& compiledShader)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

		const AssetMetadata* assetMetadata = AssetManager::GetAssetMetadata(shaderHandle);
		FLARE_CORE_ASSERT(assetMetadata);

		std::filesystem::path cacheDirectory = GetCacheDirectoryPath();

		if (!std::filesystem::exists(cacheDirectory))
			std::filesystem::create_directories(cacheDirectory);

		std::filesystem::path cacheFilePath = cacheDirectory / GetCacheFileName(shaderHandle, stageType);

		std::ofstream output(cacheFilePath, std::ios::out | std::ios::binary);
		output.write((const char*)compiledShader.data(), compiledShader.size() * sizeof(uint32_t));
		output.close();
	}

	std::optional<std::vector<uint32_t>> EditorShaderCache::FindCache(AssetHandle shaderHandle, ShaderStageType stageType)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

		const AssetMetadata* assetMetadata = AssetManager::GetAssetMetadata(shaderHandle);
		FLARE_CORE_ASSERT(assetMetadata);

		std::filesystem::path cacheDirectory = GetCacheDirectoryPath();
		std::filesystem::path cacheFilePath = cacheDirectory / GetCacheFileName(shaderHandle, stageType);

		std::vector<uint32_t> compiledShader;
		std::ifstream inputStream(cacheFilePath, std::ios::in | std::ios::binary);

		if (!inputStream.is_open())
			return {};

		inputStream.seekg(0, std::ios::end);
		size_t size = inputStream.tellg();
		inputStream.seekg(0, std::ios::beg);

		compiledShader.resize(size / sizeof(uint32_t));
		inputStream.read((char*)compiledShader.data(), size);

		return compiledShader;
	}

	Ref<const ShaderMetadata> EditorShaderCache::FindShaderMetadata(AssetHandle shaderHandle)
	{
		auto it = m_Entries.find(shaderHandle);
		if (it == m_Entries.end())
			return nullptr;
		return it->second;
	}

	Ref<const ComputeShaderMetadata> EditorShaderCache::FindComputeShaderMetadata(AssetHandle shaderHandle)
	{
		auto it = m_ComputeShaderEntries.find(shaderHandle);
		if (it == m_ComputeShaderEntries.end())
			return nullptr;
		return it->second;
	}

	bool EditorShaderCache::HasCache(AssetHandle shaderHandle, ShaderStageType stage)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

		const AssetMetadata* assetMetadata = AssetManager::GetAssetMetadata(shaderHandle);
		FLARE_CORE_ASSERT(assetMetadata);

		std::filesystem::path cacheFile = GetCacheDirectoryPath() / GetCacheFileName(shaderHandle, stage);

		return std::filesystem::exists(cacheFile);
	}

	void EditorShaderCache::SetShaderEntry(AssetHandle shaderHandle, Ref<const ShaderMetadata> metadata)
	{
		m_Entries[shaderHandle] = metadata;
	}

	void EditorShaderCache::SetComputeShaderEntry(AssetHandle shaderHandle, Ref<const ComputeShaderMetadata> metadata)
	{
		m_ComputeShaderEntries[shaderHandle] = metadata;
	}

	std::filesystem::path EditorShaderCache::GetCacheDirectoryPath()
	{
		FLARE_PROFILE_FUNCTION();

		std::string_view apiName = "";
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::Vulkan:
			apiName = "Vulkan";
			break;
		}

		return Project::GetActive()->Location / "Cache/Shaders/" / apiName;
	}

	std::string EditorShaderCache::GetCacheFileName(AssetHandle shaderHandle, ShaderStageType stageType)
	{
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));
		std::string_view shaderName = AssetManager::GetAssetMetadata(shaderHandle)->Name;
		std::string_view stageName = ShaderStageTypeToString(stageType);
		return fmt::format("{} {:x}.cache.{}", shaderName, (uint64_t)shaderHandle, stageName);
	}

	EditorShaderCache& EditorShaderCache::GetInstance()
	{
		ShaderCacheManager* instance = ShaderCacheManager::GetInstance().get();
		return *(EditorShaderCache*)instance;
	}
}
