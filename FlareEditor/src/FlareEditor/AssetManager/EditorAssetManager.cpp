#include "EditorAssetManager.h"

#include "FlareCore/Assert.h"
#include "FlareCore/Log.h"

#include "Flare/Serialization/Serialization.h"
#include "Flare/Project/Project.h"

#include "Flare/Renderer/Texture.h"
#include "Flare/Renderer/Font.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "FlareEditor/EditorLayer.h"

#include "FlareEditor/ShaderCompiler/ShaderCompiler.h"

#include "FlareEditor/AssetManager/TextureImporter.h"
#include "FlareEditor/AssetManager/PrefabImporter.h"
#include "FlareEditor/Serialization/SceneSerializer.h"
#include "FlareEditor/AssetManager/MaterialImporter.h"
#include "FlareEditor/AssetManager/MeshImporter.h"
#include "FlareEditor/AssetManager/ShaderImporter.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <string>

namespace Flare
{
    EditorAssetManager::EditorAssetManager()
    {
        m_AssetImporters.emplace(AssetType::Texture, TextureImporter::ImportTexture);
        m_AssetImporters.emplace(AssetType::Prefab, PrefabImporter::ImportPrefab);
        m_AssetImporters.emplace(AssetType::Material, MaterialImporter::ImportMaterial);
        m_AssetImporters.emplace(AssetType::Mesh, MeshImporter::ImportMesh);
        m_AssetImporters.emplace(AssetType::Scene, [](const AssetMetadata& metadata) -> Ref<Asset>
        {
            Ref<Scene> scene = CreateRef<Scene>(EditorLayer::GetInstance().GetECSContext());
            SceneSerializer::Deserialize(scene, metadata.Path);
            return scene;
        });

        m_AssetImporters.emplace(AssetType::Shader, ShaderImporter::ImportShader);

#if 0
        m_AssetImporters.emplace(AssetType::Shader, [this](const AssetMetadata& metadata) -> Ref<Asset>
        {
            std::filesystem::path assetsPath = Project::GetActive()->Location / "Assets";

            const auto& entry = m_Registry[metadata.Handle];
            if (entry.OwnerType == AssetOwner::Project)
            {
                std::filesystem::path cacheDirectory = Project::GetActive()->Location
                    / "Cache/Shaders/"
                    / std::filesystem::relative(metadata.Path.parent_path(), assetsPath);
                return Shader::Create(metadata.Path, cacheDirectory);
            }
            else if (entry.OwnerType == AssetOwner::Package)
            {
                FLARE_CORE_ASSERT(entry.PackageId.has_value());
                const AssetsPackage& package = m_AssetPackages[entry.PackageId.value()];
                std::filesystem::path packageAssetsPath = std::filesystem::absolute(AssetsPackage::InternalPackagesLocation / package.Name / "Assets");

                std::filesystem::path cacheDirectory = Project::GetActive()->Location
                    / "Cache/Shaders/"
                    / std::filesystem::relative(
                        std::filesystem::absolute(metadata.Path)
                        .parent_path(),
                        packageAssetsPath) / package.Name;

                Ref<Shader> shader = Shader::Create(metadata.Path, cacheDirectory);
                shader->Handle = metadata.Handle;
                shader->Load();

                return shader;
            }

            return nullptr;
        });
#endif

        m_AssetImporters.emplace(AssetType::Font, [](const AssetMetadata& metadata) -> Ref<Asset>
        {
            return CreateRef<Font>(metadata.Path);
        });
    }

    void EditorAssetManager::Reinitialize()
    {
        m_LoadedAssets.clear();
        m_Registry.clear();
        m_AssetPackages.clear();
        m_FilepathToAssetHandle.clear();

        ShaderLibrary::Clear();

        FLARE_CORE_ASSERT(Project::GetActive());

        std::filesystem::path root = AssetRegistrySerializer::GetAssetsRoot();
        if (!std::filesystem::exists(root))
            std::filesystem::create_directories(root);

        DeserializeRegistry();

        static std::filesystem::path internalPackagesLocation = "../Packages";

        std::filesystem::path packagesFile = Project::GetActive()->Location / PackageDependenciesSerializer::PackagesFileName;
        if (std::filesystem::exists(packagesFile))
        {
            PackageDependenciesSerializer::Deserialize(m_AssetPackages, Project::GetActive()->Location);

            for (const auto& [id, package] : m_AssetPackages)
            {
                if (package.PackageType == AssetsPackage::Type::Internal)
                {
                    AssetRegistrySerializer::Deserialize(m_Registry, internalPackagesLocation / package.Name, id);

                    for (const auto& [handle, entry] : m_Registry)
                        m_FilepathToAssetHandle.emplace(entry.Metadata.Path, handle);
                }
            }
        }

        for (const auto& entry : m_Registry)
        {
            if (entry.second.Metadata.Type == AssetType::Shader)
            {
                std::string shaderFileName = entry.second.Metadata.Path.filename().string();
                size_t dotPosition = shaderFileName.find_first_of(".");

                std::string_view shaderName = shaderFileName;
                if (dotPosition != std::string_view::npos)
                    shaderName = std::string_view(shaderFileName).substr(0, dotPosition);

                ShaderLibrary::AddShader(shaderName, entry.second.Metadata.Handle);
            }
        }
    }

    Ref<Asset> EditorAssetManager::GetAsset(AssetHandle handle)
    {
        auto assetIterator = m_LoadedAssets.find(handle);
        if (assetIterator != m_LoadedAssets.end())
            return assetIterator->second;

        auto registryIterator = m_Registry.find(handle);
        if (registryIterator == m_Registry.end())
            return nullptr;

        return LoadAsset(registryIterator->second.Metadata);
    }

    const AssetMetadata* EditorAssetManager::GetAssetMetadata(AssetHandle handle)
    {
        auto it = m_Registry.find(handle);
        if (it == m_Registry.end())
            return nullptr;
        return &it->second.Metadata;
    }

    bool EditorAssetManager::IsAssetHandleValid(AssetHandle handle)
    {
        return m_Registry.find(handle) != m_Registry.end();
    }

    bool EditorAssetManager::IsAssetLoaded(AssetHandle handle)
    {
        return m_LoadedAssets.find(handle) != m_LoadedAssets.end();
    }

    std::optional<AssetHandle> EditorAssetManager::FindAssetByPath(const std::filesystem::path& path)
    {
        auto it = m_FilepathToAssetHandle.find(path);
        if (it == m_FilepathToAssetHandle.end())
            return {};
        return it->second;
    }

    AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& path)
    {
        AssetType type = AssetType::None;
        std::filesystem::path extension = path.extension();

        if (extension == ".png")
            type = AssetType::Texture;
        else if (extension == ".flare")
            type = AssetType::Scene;
        else if (extension == ".flrprefab")
            type = AssetType::Prefab;
        else if (extension == ".flrmat")
            type = AssetType::Material;
        else if (extension == ".glsl")
            type = AssetType::Shader;
        else if (extension == ".ttf")
            type = AssetType::Font;
        else if (extension == ".fbx" || extension == ".gltf")
            type = AssetType::Mesh;

        AssetHandle handle;

        AssetRegistryEntry entry;
        entry.OwnerType = AssetOwner::Project;
        entry.PackageId = {};

        AssetMetadata& metadata = entry.Metadata;
        metadata.Path = path;
        metadata.Type = type;
        metadata.Handle = handle;

        m_Registry.emplace(handle, entry);
        m_FilepathToAssetHandle.emplace(path, handle);

        SerializeRegistry();

        return handle;
    }

    AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& path, const Ref<Asset> asset)
    {
        AssetHandle handle;

        AssetRegistryEntry entry;
        entry.OwnerType = AssetOwner::Project;
        entry.PackageId = {};

        AssetMetadata& metadata = entry.Metadata;
        metadata.Handle = handle;
        metadata.Path = path;
        metadata.Type = asset->GetType();

        asset->Handle = handle;

        m_Registry.emplace(handle, entry);
        m_LoadedAssets.emplace(asset->Handle, asset);
        m_FilepathToAssetHandle.emplace(path, handle);

        SerializeRegistry();

        return handle;
    }

    void EditorAssetManager::ReloadAsset(AssetHandle handle)
    {
        FLARE_CORE_ASSERT(IsAssetHandleValid(handle));

        auto registryIterator = m_Registry.find(handle);
        if (registryIterator == m_Registry.end())
            return;

        if (registryIterator->second.Metadata.Type == AssetType::Shader)
        {
            ShaderCompiler::Compile(handle, true);

            Ref<Shader> shader = As<Shader>(m_LoadedAssets[handle]);
            shader->Load();
            return;
        }

        LoadAsset(registryIterator->second.Metadata);
    }

    void EditorAssetManager::UnloadAsset(AssetHandle handle)
    {
        auto it = m_LoadedAssets.find(handle);
        if (it == m_LoadedAssets.end())
            return;

        m_LoadedAssets.erase(it);
    }

    void EditorAssetManager::ReloadPrefabs()
    {
        for (const auto& [handle, asset] : m_Registry)
        {
            if (asset.Metadata.Type == AssetType::Prefab && IsAssetLoaded(handle))
                ReloadAsset(handle);
        }
    }

    void EditorAssetManager::RemoveFromRegistry(AssetHandle handle)
    {
        auto it = m_Registry.find(handle);
        if (it != m_Registry.end())
        {
            UnloadAsset(handle);
            m_Registry.erase(handle);
            SerializeRegistry();
        }
    }

    void EditorAssetManager::AddAssetsPackage(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
            return;

        {
            AssetsPackage package;
            package.Id = UUID();
            package.PackageType = AssetsPackage::Type::Internal;

            PackageDependenciesSerializer::DeserializePackage(package, path);

            m_AssetPackages.emplace(package.Id, std::move(package));
        }

        PackageDependenciesSerializer::Serialize(m_AssetPackages, Project::GetActive()->Location);
        AssetRegistrySerializer::Deserialize(m_Registry, path.parent_path());
    }

    Ref<Asset> EditorAssetManager::LoadAsset(const AssetMetadata& metadata)
    {
        auto importerIterator = m_AssetImporters.find(metadata.Type);
        if (importerIterator == m_AssetImporters.end())
        {
            FLARE_CORE_ASSERT("Cannot import '{0}', because an imported for that asset type is not provided");
            return nullptr;
        }

        Ref<Asset> asset = importerIterator->second(metadata);
        if (!asset)
            return nullptr;

        asset->Handle = metadata.Handle;
        m_LoadedAssets[metadata.Handle] = asset;
        return asset;
    }

    void EditorAssetManager::SerializeRegistry()
    {
        AssetRegistrySerializer::Serialize(m_Registry, Project::GetActive()->Location);
    }

    void EditorAssetManager::DeserializeRegistry()
    {
        m_Registry.clear();
        AssetRegistrySerializer::Deserialize(m_Registry, Project::GetActive()->Location);

        for (const auto& [handle, entry] : m_Registry)
            m_FilepathToAssetHandle.emplace(entry.Metadata.Path, handle);
    }
}
