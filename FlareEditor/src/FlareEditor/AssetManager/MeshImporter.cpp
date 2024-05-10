#include "MeshImporter.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/MaterialsTable.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Renderer.h"

#include "FlareEditor/AssetManager/EditorAssetManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

namespace Flare
{
    struct SceneData
    {
        IndexBuffer::IndexFormat IndexFormat = IndexBuffer::IndexFormat::UInt32;

        std::vector<uint16_t> Indices16;
        std::vector<uint32_t> Indices32;

        std::vector<glm::vec3> Vertices;
        std::vector<glm::vec3> Normals;
        std::vector<glm::vec3> Tangents;
        std::vector<glm::vec2> UVs;

        size_t MaxSubMeshIndexCount = 0;

        std::vector<SubMesh> SubMeshes;
        std::vector<uint32_t> UsedMaterials;
    };

    static bool ProcessMeshNode(aiNode* node, const aiScene* scene, SceneData& data)
    {
        FLARE_PROFILE_FUNCTION();
        if (node->mNumMeshes > 0)
        {
            size_t verticesCount = 0;
            size_t indexCount = 0;
            size_t subMeshes = node->mNumMeshes;

            for (uint32_t i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh* nodeMesh = scene->mMeshes[node->mMeshes[i]];
                verticesCount += nodeMesh->mNumVertices;

                for (uint32_t face = 0; face < nodeMesh->mNumFaces; face++)
                {
                    aiFace& f = nodeMesh->mFaces[face];
                    indexCount += (size_t)f.mNumIndices;
                }
            }

            if (indexCount < (size_t)std::numeric_limits<uint16_t>::max())
            {
                data.IndexFormat = IndexBuffer::IndexFormat::UInt16;
                data.Indices16.reserve(indexCount);
            }
            else
            {
                data.Indices32.reserve(indexCount);
            }

            data.Vertices.resize(verticesCount);
            data.Normals.resize(verticesCount);
            data.Tangents.resize(verticesCount);
            data.UVs.resize(verticesCount);

            data.SubMeshes.reserve(subMeshes);

            size_t vertexOffset = 0;
            size_t indexOffset = 0;

            for (uint32_t i = 0; i < node->mNumMeshes; i++)
            {
                aiMesh* nodeMesh = scene->mMeshes[node->mMeshes[i]];
                data.UsedMaterials.push_back(nodeMesh->mMaterialIndex);

                std::memcpy(data.Vertices.data() + vertexOffset, nodeMesh->mVertices, sizeof(glm::vec3) * nodeMesh->mNumVertices);
                std::memcpy(data.Normals.data() + vertexOffset, nodeMesh->mNormals, sizeof(glm::vec3) * nodeMesh->mNumVertices);
                std::memcpy(data.Tangents.data() + vertexOffset, nodeMesh->mTangents, sizeof(glm::vec3) * nodeMesh->mNumVertices);

                if (nodeMesh->mTextureCoords != nullptr && nodeMesh->mTextureCoords[0] != nullptr)
                {
                    for (size_t i = 0; i < (size_t)nodeMesh->mNumVertices; i++)
                    {
                        auto uv = nodeMesh->mTextureCoords[0][i];
                        data.UVs[i + vertexOffset].x = uv.x;
                        data.UVs[i + vertexOffset].y = uv.y;
                    }
                }
                else
                {
                    for (size_t i = 0; i < (size_t)nodeMesh->mNumVertices; i++)
                        data.UVs[i + vertexOffset] = glm::vec2(0.0f);
                }

                size_t subMeshIndexCount = 0;
                if (data.IndexFormat == IndexBuffer::IndexFormat::UInt16)
                {
                    for (uint32_t face = 0; face < nodeMesh->mNumFaces; face++)
                    {
                        aiFace& f = nodeMesh->mFaces[face];
                        subMeshIndexCount += f.mNumIndices;

                        for (uint32_t i = 0; i < f.mNumIndices; i++)
                            data.Indices16.push_back((uint16_t)f.mIndices[i]);
                    }
                }
                else
                {
                    for (uint32_t face = 0; face < nodeMesh->mNumFaces; face++)
                    {
                        aiFace& f = nodeMesh->mFaces[face];
                        subMeshIndexCount += f.mNumIndices;

                        for (uint32_t i = 0; i < f.mNumIndices; i++)
                            data.Indices32.push_back((uint32_t)f.mIndices[i]);
                    }
                }

                auto& subMesh = data.SubMeshes.emplace_back();
                subMesh.BaseVertex = (uint32_t)vertexOffset;
                subMesh.BaseIndex = (uint32_t)indexOffset;
                subMesh.IndicesCount = (uint32_t)subMeshIndexCount;

                subMesh.Bounds.Min = data.Vertices[vertexOffset];
                subMesh.Bounds.Max = data.Vertices[vertexOffset];
                for (size_t i = vertexOffset + 1; i < vertexOffset + (size_t)nodeMesh->mNumVertices; i++)
                {
                    subMesh.Bounds.Min = glm::min(data.Vertices[i], subMesh.Bounds.Min);
                    subMesh.Bounds.Max = glm::max(data.Vertices[i], subMesh.Bounds.Max);
                }

                vertexOffset += nodeMesh->mNumVertices;
                indexOffset += subMeshIndexCount;
            }

            return true;
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            bool result = ProcessMeshNode(node->mChildren[i], scene, data);
            if (result)
            {
                return true;
            }
        }

        return false;
    }

    static AssetHandle FindTextureByPath(std::string_view path, const AssetMetadata& metadata, const Ref<EditorAssetManager>& assetManager)
    {
        FLARE_PROFILE_FUNCTION();
        AssetHandle textureHandle = NULL_ASSET_HANDLE;
        if (path.size() > 0)
        {
            std::filesystem::path texturePath = metadata.Path.parent_path() / path;
            if (std::filesystem::exists(texturePath))
            {
                std::optional<AssetHandle> handle = assetManager->FindAssetByPath(texturePath);
                if (handle)
                    textureHandle = handle.value();
                else
                    textureHandle = assetManager->ImportAsset(texturePath);
            }
        }

        return textureHandle;
    }

    static void TrySetMaterialTexture(std::optional<uint32_t> propertyIndex, const Ref<Material>& material, AssetHandle handle, const Ref<Texture>& defaultValue)
    {
        FLARE_PROFILE_FUNCTION();
        if (propertyIndex)
        {
            auto& value = material->GetPropertyValue<TexturePropertyValue>(*propertyIndex);

            if (handle == NULL_ASSET_HANDLE)
                value.SetTexture(defaultValue);
            else
                value.SetTexture(AssetManager::GetAsset<Texture>(handle));
        }
    }

    static void ImportMaterials(const AssetMetadata& metadata, const aiScene* scene, const std::vector<uint32_t>& usedMaterials)
    {
        FLARE_PROFILE_FUNCTION();

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
        std::optional<uint32_t> normalMapProperty;
        std::optional<uint32_t> roughnessMapProperty;

        Ref<Shader> shader = AssetManager::GetAsset<Shader>(defaultShader.value());
        if (shader != nullptr && shader->IsLoaded())
        {
            colorProperty = shader->GetPropertyIndex("u_Material.Color");
            roughnessProperty = shader->GetPropertyIndex("u_Material.Roughness");
            textureProperty = shader->GetPropertyIndex("u_Texture");
            normalMapProperty = shader->GetPropertyIndex("u_NormalMap");
            roughnessMapProperty = shader->GetPropertyIndex("u_RoughnessMap");
        }

        Ref<MaterialsTable> materialsTable = CreateRef<MaterialsTable>();
        if (AssetManager::IsAssetHandleValid(materialsTableHandle))
            assetManager->SetLoadedAsset(materialsTableHandle, materialsTable);
        else
            assetManager->ImportMemoryOnlyAsset("DefaultMaterialsTable", materialsTable, metadata.Handle);

        materialsTable->Materials.reserve(usedMaterials.size());

        auto getMaterialTexture = [&](const aiMaterial& material, aiTextureType type) -> AssetHandle
        {
            FLARE_PROFILE_FUNCTION();

            aiTextureMapping mapping;
            uint32_t uvIndex;
            aiString path;

            aiReturn result = material.GetTexture(type, 0, &path, &mapping, &uvIndex);
            if (result == aiReturn_SUCCESS)
                return FindTextureByPath(std::string_view(path.C_Str(), path.length), metadata, assetManager);

            return NULL_ASSET_HANDLE;
        };

        for (uint32_t i : usedMaterials)
        {
            FLARE_PROFILE_SCOPE("ImportSingleMaterial");
            auto& material = scene->mMaterials[i];

            std::string name = material->GetName().C_Str();
            if (name.empty())
                name = fmt::format("Material {}", i);

            AssetHandle baseColorTextureHandle = NULL_ASSET_HANDLE;
            AssetHandle normalMapHandle = NULL_ASSET_HANDLE;
            AssetHandle roughnessMapHandle = NULL_ASSET_HANDLE;

            baseColorTextureHandle = getMaterialTexture(*material, aiTextureType_BASE_COLOR);
            normalMapHandle = getMaterialTexture(*material, aiTextureType_NORMALS);
            roughnessMapHandle = getMaterialTexture(*material, aiTextureType_DIFFUSE_ROUGHNESS);

            if (baseColorTextureHandle == NULL_ASSET_HANDLE)
            {
                baseColorTextureHandle = getMaterialTexture(*material, aiTextureType_DIFFUSE);
            }

            aiColor4D color(1.0f, 1.0f, 1.0f, 1.0f);
            material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            float roughness = 1.0f;
            material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

            Ref<Material> materialAsset = Material::Create(defaultShader.value());

            if (colorProperty)
                materialAsset->WritePropertyValue(*colorProperty, glm::vec4(color.r, color.g, color.b, color.a));
            if (roughnessProperty)
                materialAsset->WritePropertyValue(*roughnessProperty, roughness);

            TrySetMaterialTexture(textureProperty, materialAsset, baseColorTextureHandle, Renderer::GetWhiteTexture());
            TrySetMaterialTexture(normalMapProperty, materialAsset, normalMapHandle, Renderer::GetDefaultNormalMap());
            TrySetMaterialTexture(roughnessMapProperty, materialAsset, roughnessMapHandle, Renderer::GetWhiteTexture());

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
        FLARE_PROFILE_FUNCTION();
        std::underlying_type_t<aiPostProcessSteps> postProcessSteps = aiProcess_Triangulate | aiProcess_CalcTangentSpace | aiProcess_FlipWindingOrder;
        if (metadata.Path.extension() == ".fbx")
            postProcessSteps |= aiProcess_FlipUVs;

        Assimp::Importer importer;
        const aiScene* scene = nullptr;

		{
            FLARE_PROFILE_SCOPE("ReadFile");
			scene = importer.ReadFile(metadata.Path.string(), postProcessSteps);
        }

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            FLARE_CORE_ERROR("Failed to load mesh {}: {}", metadata.Path.generic_string(), importer.GetErrorString());
            return nullptr;
        }

        SceneData data;

        bool result = ProcessMeshNode(scene->mRootNode, scene, data);
        FLARE_CORE_ASSERT(result);

        MemorySpan indices = MemorySpan();
        if (data.IndexFormat == IndexBuffer::IndexFormat::UInt16)
        {
            indices = MemorySpan::FromVector(data.Indices16);
        }
        else
        {
            indices = MemorySpan::FromVector(data.Indices32);
        }

        Ref<Mesh> mesh = Mesh::Create(MeshTopology::Triangles, indices, data.IndexFormat,
            Span<glm::vec3>::FromVector(data.Vertices),
            Span<glm::vec3>::FromVector(data.Normals),
            Span<glm::vec3>::FromVector(data.Tangents),
            Span<glm::vec2>::FromVector(data.UVs));

        for (const auto& subMesh : data.SubMeshes)
        {
            mesh->AddSubMesh(subMesh);
        }

        ImportMaterials(metadata, scene, data.UsedMaterials);

        return mesh;
    }
}
