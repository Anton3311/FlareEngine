#include "StaticMeshImporter.h"

#include "FlareCore/Log.h"
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
		WalkHierarchy(m_Scene->mRootNode, glm::mat4(1.0f));
	}

	inline static glm::mat4 ConvertToColumnMajor(const aiMatrix4x4& matrix)
	{
		return glm::mat4(
			matrix.a1, matrix.b1, matrix.c1, matrix.d1,
			matrix.a2, matrix.b2, matrix.c2, matrix.d2,
			matrix.a3, matrix.b3, matrix.c3, matrix.d3,
			matrix.d4, matrix.b4, matrix.c4, matrix.d4);
	}

	void StaticMeshImporter::WalkHierarchy(const aiNode* node, const glm::mat4& parentTransform)
	{
		FLARE_PROFILE_FUNCTION();

		glm::mat4 nodeTransform = parentTransform * ConvertToColumnMajor(node->mTransformation);

		for (uint32_t i = 0; i < node->mNumChildren; i++)
		{
			VisitNode(node->mChildren[i], parentTransform);
			WalkHierarchy(node->mChildren[i], parentTransform);
		}
	}

	void StaticMeshImporter::VisitNode(const aiNode* node, const glm::mat4& parentTransform)
	{
		FLARE_PROFILE_FUNCTION();

		if (node->mNumMeshes == 0)
			return;

		glm::mat4 nodeTransform = parentTransform * ConvertToColumnMajor(node->mTransformation);

		for (uint32_t i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* nodeMesh = m_Scene->mMeshes[node->mMeshes[i]];
			m_SceneData.UsedMaterials.push_back(nodeMesh->mMaterialIndex);

			size_t subMeshStart = m_VertexOffset;
			size_t subMeshEnd = m_VertexOffset + (size_t)nodeMesh->mNumVertices;

			CopySubMeshData(nodeMesh);
			FlattenHierarchy(node, nodeTransform, subMeshStart, subMeshEnd);

			{
				FLARE_PROFILE_SCOPE("CalculateBounds");
				SubMesh& subMesh = m_SceneData.SubMeshes.back();
				subMesh.Bounds.Min = m_SceneData.Vertices[subMeshStart];
				subMesh.Bounds.Max = m_SceneData.Vertices[subMeshStart];
				for (size_t i = subMeshStart + 1; i < subMeshEnd; i++)
				{
					subMesh.Bounds.Min = glm::min(m_SceneData.Vertices[i], subMesh.Bounds.Min);
					subMesh.Bounds.Max = glm::max(m_SceneData.Vertices[i], subMesh.Bounds.Max);
				}
			}
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

		m_VertexOffset += mesh->mNumVertices;
		m_IndexOffset += subMeshIndexCount;
	}

	void StaticMeshImporter::FlattenHierarchy(const aiNode* node, const glm::mat4& transform, size_t subMeshStart, size_t subMeshEnd)
	{
		FLARE_PROFILE_FUNCTION();

		for (size_t i = subMeshStart; i < subMeshEnd; i++)
		{
			m_SceneData.Vertices[i] = transform * glm::vec4(m_SceneData.Vertices[i], 1.0f);
		}

		for (size_t i = subMeshStart; i < subMeshEnd; i++)
		{
			m_SceneData.Normals[i] = transform * glm::vec4(m_SceneData.Normals[i], 0.0f);
		}

		for (size_t i = subMeshStart; i < subMeshEnd; i++)
		{
			m_SceneData.Tangents[i] = transform * glm::vec4(m_SceneData.Tangents[i], 0.0f);
		}
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
