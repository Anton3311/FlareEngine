#include "MeshImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Flare
{
	static Ref<Mesh> ProcessMeshNode(aiNode* node, const aiScene* scene)
	{
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* nodeMesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<glm::vec3> vertices;
			std::vector<glm::vec3> normals;
			std::vector<uint32_t> indices;

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

			for (int32_t face = 0; face < nodeMesh->mNumFaces; face++)
			{
				aiFace& f = nodeMesh->mFaces[face];

				size_t start = indices.size();
				for (int32_t i = 0; i < f.mNumIndices; i++)
					indices.push_back(f.mIndices[i]);

				// Swap winding order
				std::swap(indices[start], indices[start + 1]);
			}

			// TODO: import all meshes, when the submeshes are fully implemented
			// Import the first mesh for now.
			return CreateRef<Mesh>(vertices.data(), vertices.size(),
				indices.data(), indices.size(),
				normals.data());
		}

		for (int i = 0; i < node->mNumChildren; i++)
		{
			Ref<Mesh> mesh = ProcessMeshNode(node->mChildren[i], scene);
			if (mesh != nullptr)
				return mesh;
		}

		return nullptr;
	}

    Ref<Mesh> MeshImporter::ImportMesh(const AssetMetadata& metadata)
    {
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(metadata.Path.string(), aiProcess_Triangulate | aiProcess_FlipUVs);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
			FLARE_CORE_ERROR("Failed to load mesh {}: {}", metadata.Path.generic_string(), importer.GetErrorString());
		else
			return ProcessMeshNode(scene->mRootNode, scene);

		return nullptr;
    }
}
