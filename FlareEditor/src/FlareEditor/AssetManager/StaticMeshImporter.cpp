#include "StaticMeshImporter.h"

#include "FlareCore/Profiler/Profiler.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

namespace Flare
{
	void StaticMeshImporter::Import()
	{
		FLARE_PROFILE_FUNCTION();

		ReserveBuffers();
		WalkHierarchy(m_Scene->mRootNode);
	}

	void StaticMeshImporter::WalkHierarchy(const aiNode* node)
	{
		FLARE_PROFILE_FUNCTION();

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			VisitNode(node->mChildren[i]);
			WalkHierarchy(node->mChildren[i]);
		}
	}

	void StaticMeshImporter::VisitNode(const aiNode* node)
	{
		FLARE_PROFILE_FUNCTION();

		if (node->mNumMeshes == 0)
			return;

		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* nodeMesh = m_Scene->mMeshes[node->mMeshes[i]];
			m_SceneData.UsedMaterials.push_back(nodeMesh->mMaterialIndex);

			CopySubMeshData(nodeMesh);

			node->mTransformation;
		}
	}

	void StaticMeshImporter::CopySubMeshData(const aiMesh* mesh)
	{
		FLARE_PROFILE_FUNCTION();

		std::memcpy(m_SceneData.Vertices.data() + m_VertexOffset, mesh->mVertices, sizeof(glm::vec3) * mesh->mNumVertices);
		std::memcpy(m_SceneData.Normals.data() + m_VertexOffset, mesh->mNormals, sizeof(glm::vec3) * mesh->mNumVertices);
		std::memcpy(m_SceneData.Tangents.data() + m_VertexOffset, mesh->mTangents, sizeof(glm::vec3) * mesh->mNumVertices);

		if (mesh->mTextureCoords != nullptr && mesh->mTextureCoords[0] != nullptr)
		{
			for (size_t i = 0; i < (size_t)mesh->mNumVertices; i++)
			{
				auto uv = mesh->mTextureCoords[0][i];
				m_SceneData.UVs[i + m_VertexOffset].x = uv.x;
				m_SceneData.UVs[i + m_VertexOffset].y = uv.y;
			}
		}
		else
		{
			for (size_t i = 0; i < (size_t)mesh->mNumVertices; i++)
				m_SceneData.UVs[i + m_VertexOffset] = glm::vec2(0.0f);
		}

		size_t subMeshIndexCount = 0;
		if (m_SceneData.IndexFormat == IndexBuffer::IndexFormat::UInt16)
		{
			for (uint32_t face = 0; face < mesh->mNumFaces; face++)
			{
				aiFace& f = mesh->mFaces[face];
				subMeshIndexCount += f.mNumIndices;

				for (uint32_t i = 0; i < f.mNumIndices; i++)
					m_SceneData.Indices16.push_back((uint16_t)f.mIndices[i] + (uint16_t)m_VertexOffset);
			}
		}
		else
		{
			for (uint32_t face = 0; face < mesh->mNumFaces; face++)
			{
				aiFace& f = mesh->mFaces[face];
				subMeshIndexCount += f.mNumIndices;

				for (uint32_t i = 0; i < f.mNumIndices; i++)
					m_SceneData.Indices32.push_back((uint32_t)f.mIndices[i] + (uint32_t)m_VertexOffset);
			}
		}

		auto& subMesh = m_SceneData.SubMeshes.emplace_back();
		subMesh.BaseVertex = 0;
		subMesh.BaseIndex = (uint32_t)m_IndexOffset;
		subMesh.IndicesCount = (uint32_t)subMeshIndexCount;
		subMesh.Bounds.Min = m_SceneData.Vertices[m_VertexOffset];
		subMesh.Bounds.Max = m_SceneData.Vertices[m_VertexOffset];
		for (size_t i = m_VertexOffset + 1; i < m_VertexOffset + (size_t)mesh->mNumVertices; i++)
		{
			subMesh.Bounds.Min = glm::min(m_SceneData.Vertices[i], subMesh.Bounds.Min);
			subMesh.Bounds.Max = glm::max(m_SceneData.Vertices[i], subMesh.Bounds.Max);
		}

		m_VertexOffset += mesh->mNumVertices;
		m_IndexOffset += subMeshIndexCount;
	}

	void StaticMeshImporter::ReserveBuffers()
	{
		FLARE_PROFILE_FUNCTION();

		size_t vertexCount = 0;
		size_t indexCount = 0;
		CountVerticesAndIndicesRecursively(m_Scene->mRootNode, vertexCount, indexCount);

		m_SceneData.Vertices.resize(vertexCount);
		m_SceneData.Normals.resize(vertexCount);
		m_SceneData.Tangents.resize(vertexCount);
		m_SceneData.UVs.resize(vertexCount);

		if (indexCount <= (size_t)std::numeric_limits<uint16_t>::max())
		{
			m_SceneData.IndexFormat = IndexBuffer::IndexFormat::UInt16;
			m_SceneData.Indices16.reserve(indexCount);
		}
		else
		{
			m_SceneData.IndexFormat = IndexBuffer::IndexFormat::UInt32;
			m_SceneData.Indices32.reserve(indexCount);
		}
	}

	void StaticMeshImporter::CountVerticesAndIndicesRecursively(const aiNode* node, size_t& vertexCount, size_t& indexCount)
	{
		FLARE_PROFILE_FUNCTION();

		CountVerticesAndIndices(node, vertexCount, indexCount);

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			CountVerticesAndIndicesRecursively(node->mChildren[i], vertexCount, indexCount);
		}
	}

	void StaticMeshImporter::CountVerticesAndIndices(const aiNode* node, size_t& vertexCount, size_t& indexCount)
	{
		FLARE_PROFILE_FUNCTION();
		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* nodeMesh = m_Scene->mMeshes[node->mMeshes[i]];
			vertexCount += nodeMesh->mNumVertices;

			for (uint32_t face = 0; face < nodeMesh->mNumFaces; face++)
			{
				aiFace& f = nodeMesh->mFaces[face];
				indexCount += (size_t)f.mNumIndices;
			}
		}
	}

}
