#include "EditorShaderCache.h"

#include "FlareCore/Core.h"

#include "Flare/AssetManager/AssetManager.h"
#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <fstream>

namespace Flare
{
    void EditorShaderCache::SetCache(AssetHandle shaderHandle,
        ShaderTargetEnvironment targetEnvironment,
        ShaderStageType stageType,
        const std::vector<uint32_t>& compiledShader)
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

        const AssetMetadata* assetMetadata = AssetManager::GetAssetMetadata(shaderHandle);
        FLARE_CORE_ASSERT(assetMetadata);

        std::filesystem::path cacheDirectory = GetCacheDirectoryPath(shaderHandle);

        if (!std::filesystem::exists(cacheDirectory))
            std::filesystem::create_directories(cacheDirectory);

        std::filesystem::path cacheFilePath = cacheDirectory / GetCacheFileName(
            assetMetadata->Path.filename().string(),
            stageType);

        std::ofstream output(cacheFilePath, std::ios::out | std::ios::binary);
        output.write((const char*)compiledShader.data(), compiledShader.size() * sizeof(uint32_t));
        output.close();
    }

    std::optional<std::vector<uint32_t>> EditorShaderCache::FindCache(AssetHandle shaderHandle,
        ShaderTargetEnvironment targetEnvironment,
        ShaderStageType stageType)
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

        const AssetMetadata* assetMetadata = AssetManager::GetAssetMetadata(shaderHandle);
        FLARE_CORE_ASSERT(assetMetadata);

        std::filesystem::path cacheDirectory = GetCacheDirectoryPath(shaderHandle);
        std::filesystem::path cacheFilePath = cacheDirectory / GetCacheFileName(
            assetMetadata->Path.filename().string(),
            stageType);

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

    bool EditorShaderCache::HasCache(AssetHandle shaderHandle, ShaderTargetEnvironment targetEnvironment, ShaderStageType stage)
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

        const AssetMetadata* assetMetadata = AssetManager::GetAssetMetadata(shaderHandle);
        FLARE_CORE_ASSERT(assetMetadata);

        std::filesystem::path cacheFile = GetCacheDirectoryPath(shaderHandle)
            / GetCacheFileName(assetMetadata->Path.filename().string(), stage);

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

    std::filesystem::path EditorShaderCache::GetCacheDirectoryPath(AssetHandle shaderHandle)
    {
        FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(shaderHandle));

        Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
        const auto& registry = assetManager->GetRegistry();
        const auto& packages = assetManager->GetAssetPackages();

        auto it = registry.find(shaderHandle);
        FLARE_CORE_ASSERT(it != registry.end(), "Failed to find shader asset in the registry");

        std::string_view apiName = "";
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::OpenGL:
            apiName = "OpenGL";
            break;
        case RendererAPI::API::Vulkan:
            apiName = "Vulkan";
            break;
        }

        const auto& entry = it->second;
        std::filesystem::path cacheDirectory;
        if (entry.OwnerType == AssetOwner::Project)
        {
            std::filesystem::path assetsPath = Project::GetActive()->Location / "Assets";

            cacheDirectory = Project::GetActive()->Location
                / "Cache/Shaders/"
                / apiName
                / std::filesystem::relative(entry.Metadata.Path.parent_path(), assetsPath);
        }
        else if (entry.OwnerType == AssetOwner::Package)
        {
            FLARE_CORE_ASSERT(entry.PackageId.has_value());
            auto packageIterator = packages.find(entry.PackageId.value());
            FLARE_CORE_ASSERT(packageIterator != packages.end(), "Failed to find asset package");

            const AssetsPackage& package = packageIterator->second;
            std::filesystem::path packageAssetsPath = std::filesystem::absolute(AssetsPackage::InternalPackagesLocation / package.Name / "Assets");

            cacheDirectory = Project::GetActive()->Location
                / "Cache/Shaders/"
                / apiName
                / std::filesystem::relative(
                    std::filesystem::absolute(entry.Metadata.Path)
                    .parent_path(),
                    packageAssetsPath) / package.Name;
        }

        return cacheDirectory;
    }

    std::string EditorShaderCache::GetCacheFileName(std::string_view shaderName, ShaderStageType stageType)
    {
        std::string_view stageName = ShaderStageTypeToString(stageType);
        return fmt::format("{}.cache.{}", shaderName, stageName);
    }

    EditorShaderCache& EditorShaderCache::GetInstance()
    {
        ShaderCacheManager* instance = ShaderCacheManager::GetInstance().get();
        return *(EditorShaderCache*)instance;
    }
}
