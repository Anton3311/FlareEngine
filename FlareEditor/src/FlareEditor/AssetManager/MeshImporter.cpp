#include "MeshImporter.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/MaterialsTable.h"
#include "Flare/Renderer/ShaderLibrary.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

namespace Flare
{
    static Ref<Mesh> ProcessMeshNode(aiNode* node, const aiScene* scene, std::vector<uint32_t>& usedMaterials)
    {
        Ref<Mesh> mesh = nullptr;

        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* nodeMesh = scene->mMeshes[node->mMeshes[i]];
            usedMaterials.push_back(nodeMesh->mMaterialIndex);

            std::vector<glm::vec3> vertices;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> uvs;


            vertices.resize(nodeMesh->mNumVertices);
            normals.resize(nodeMesh->mNumVertices);

            for (size_t i = 0; i < vertices.size(); i++)
            {
                vertices[i].x = nodeMesh->mVertices[i].x;
                vertices[i].y = nodeMesh->mVertices[i].y;
                vertices[i].z = nodeMesh->mVertices[i].z;
            }

            for (size_t i = 0; i < normals.size(); i++)
            {
                normals[i].x = nodeMesh->mNormals[i].x;
                normals[i].y = nodeMesh->mNormals[i].y;
                normals[i].z = nodeMesh->mNormals[i].z;
            }

            bool hasUVs = false;
            if (nodeMesh->mTextureCoords != nullptr && nodeMesh->mTextureCoords[0] != nullptr)
            {
                hasUVs = true;
                uvs.resize(nodeMesh->mNumVertices);
                for (size_t i = 0; i < uvs.size(); i++)
                {
                    auto uv = nodeMesh->mTextureCoords[0][i];
                    uvs[i].x = uv.x;
                    uvs[i].y = uv.y;
                }
            }

            size_t indicesCount = 0;
            for (uint32_t face = 0; face < nodeMesh->mNumFaces; face++)
            {
                aiFace& f = nodeMesh->mFaces[face];
                indicesCount += (size_t)f.mNumIndices;
            }

            std::vector<uint16_t> indices16;
            std::vector<uint32_t> indices32;

            IndexBuffer::IndexFormat indexFormat = IndexBuffer::IndexFormat::UInt32;
            if (indicesCount < (size_t)std::numeric_limits<uint16_t>::max())
            {
                indexFormat = IndexBuffer::IndexFormat::UInt16;
                indices16.reserve(indicesCount);

                for (uint32_t face = 0; face < nodeMesh->mNumFaces; face++)
                {
                    aiFace& f = nodeMesh->mFaces[face];

                    size_t start = indices16.size();
                    for (uint32_t i = 0; i < f.mNumIndices; i++)
                        indices16.push_back((uint16_t)f.mIndices[i]);

                    // Swap winding order
                    std::swap(indices16[start], indices16[start + 1]);
                }
            }
            else
            {
                indexFormat = IndexBuffer::IndexFormat::UInt32;
                indices32.reserve(indicesCount);

                for (uint32_t face = 0; face < nodeMesh->mNumFaces; face++)
                {
                    aiFace& f = nodeMesh->mFaces[face];

                    size_t start = indices32.size();
                    for (uint32_t i = 0; i < f.mNumIndices; i++)
                        indices32.push_back((uint32_t)f.mIndices[i]);

                    // Swap winding order
                    std::swap(indices32[start], indices32[start + 1]);
                }
            }

            if (!mesh)
                mesh = CreateRef<Mesh>();

            Span<glm::vec3> normalsSpan = Span<glm::vec3>::FromVector(normals);
            Span<glm::vec2> uvsSpan = Span<glm::vec2>::FromVector(uvs);

            mesh->AddSubMesh(
                Span<glm::vec3>::FromVector(vertices),
                indexFormat,
                indexFormat == IndexBuffer::IndexFormat::UInt16
                    ? MemorySpan::FromVector(indices16)
                    : MemorySpan::FromVector(indices32),
                &normalsSpan,
                hasUVs ? &uvsSpan : nullptr);
        }

        if (mesh)
            return mesh;

        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            Ref<Mesh> mesh = ProcessMeshNode(node->mChildren[i], scene, usedMaterials);
            if (mesh != nullptr)
                return mesh;
        }

        return nullptr;
    }

    static void ImportMaterials(const AssetMetadata& metadata, const aiScene* scene, const std::vector<uint32_t>& usedMaterials)
    {
        Ref<EditorAssetManager> assetManager = As<EditorAssetManager>(AssetManager::GetInstance());
        std::optional<AssetHandle> defaultShader = ShaderLibrary::FindShader("Mesh");

        std::unordered_map<std::string, AssetHandle> nameToHandle;
        AssetHandle materialsTableHandle;

        for (AssetHandle subAsset : metadata.SubAssets)
        {
            if (const auto* subAssetMetadata = AssetManager::GetAssetMetadata(subAsset))
            {
                if (subAssetMetadata->Type == AssetType::Material)
                    nameToHandle[subAssetMetadata->Name] = subAsset;
                else
                    materialsTableHandle = subAsset;
            }
        }

        if (!defaultShader)
        {
            FLARE_CORE_ERROR("Failed to find 'Mesh' shader");
            return;
        }

        std::optional<uint32_t> colorProperty;
        std::optional<uint32_t> roughnessProperty;
        std::optional<uint32_t> textureProperty;

        Ref<Shader> shader = AssetManager::GetAsset<Shader>(defaultShader.value());
        if (shader != nullptr && shader->IsLoaded())
        {
            colorProperty = shader->GetPropertyIndex("u_InstanceData.Color");
            roughnessProperty = shader->GetPropertyIndex("u_InstanceData.Roughness");
            textureProperty = shader->GetPropertyIndex("u_Texture");
        }

        Ref<MaterialsTable> materialsTable = CreateRef<MaterialsTable>();
        if (AssetManager::IsAssetHandleValid(materialsTableHandle))
            assetManager->SetLoadedAsset(materialsTableHandle, materialsTable);
        else
            assetManager->ImportMemoryOnlyAsset("DefaultMaterialsTable", materialsTable, metadata.Handle);

        materialsTable->Materials.reserve(usedMaterials.size());

        for (uint32_t i : usedMaterials)
        {
            auto& material = scene->mMaterials[i];

            std::string name = material->GetName().C_Str();
            if (name.empty())
                name = fmt::format("Material {}", i);

            aiString texturePath;
            aiTextureMapping mapping;
            uint32_t uvIndex;
            material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath, &mapping, &uvIndex);

            AssetHandle baseColorTextureHandle = NULL_ASSET_HANDLE;
            if (texturePath.length > 0)
            {
                std::filesystem::path baseColorTexturePath = metadata.Path.parent_path() / texturePath.C_Str();
                if (std::filesystem::exists(baseColorTexturePath))
                {
                    std::optional<AssetHandle> handle = assetManager->FindAssetByPath(baseColorTexturePath);
                    if (handle)
                        baseColorTextureHandle = handle.value();
                    else
                        baseColorTextureHandle = assetManager->ImportAsset(baseColorTexturePath);
                }
            }

            aiColor4D color;
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            float roughness;
            material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

            Ref<Material> materialAsset = CreateRef<Material>(defaultShader.value());

            if (colorProperty)
                materialAsset->WritePropertyValue(*colorProperty, glm::vec4(color.r, color.g, color.b, color.a));
            if (roughnessProperty)
                materialAsset->WritePropertyValue(*roughnessProperty, roughness);
            if (textureProperty)
                materialAsset->WritePropertyValue(*textureProperty, baseColorTextureHandle);

            auto it = nameToHandle.find(name);
            if (it != nameToHandle.end())
            {
                assetManager->SetLoadedAsset(it->second, materialAsset);
                materialsTable->Materials.push_back(it->second);
            }
            else
            {
                AssetHandle handle = assetManager->ImportMemoryOnlyAsset(name, materialAsset, metadata.Handle);
                materialsTable->Materials.push_back(handle);
            }

        }
    }

    Ref<Mesh> MeshImporter::ImportMesh(const AssetMetadata& metadata)
    {
        std::underlying_type_t<aiPostProcessSteps> postProcessSteps = aiProcess_Triangulate;
        if (metadata.Path.extension() == ".fbx")
            postProcessSteps |= aiProcess_FlipUVs;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(metadata.Path.string(), postProcessSteps);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            FLARE_CORE_ERROR("Failed to load mesh {}: {}", metadata.Path.generic_string(), importer.GetErrorString());
            return nullptr;
        }

        std::vector<uint32_t> usedMaterials;

        Ref<Mesh> mesh = ProcessMeshNode(scene->mRootNode, scene, usedMaterials);
        ImportMaterials(metadata, scene, usedMaterials);

        return mesh;
    }
}
