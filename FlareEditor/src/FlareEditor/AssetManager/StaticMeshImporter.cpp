#include "StaticMeshImporter.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/CommandBuffer.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"

#include "FlareEditor/AssetManager/MeshImportSettings.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

namespace Flare
{
	glm::mat4 ConvertToColumnMajor(const aiMatrix4x4& matrix)
	{
		return glm::mat4(
			matrix.a1, matrix.b1, matrix.c1, matrix.d1,
			matrix.a2, matrix.b2, matrix.c2, matrix.d2,
			matrix.a3, matrix.b3, matrix.c3, matrix.d3,
			matrix.a4, matrix.b4, matrix.c4, matrix.d4);
	}

	void StaticMeshImporter::Import()
	{
		FLARE_PROFILE_FUNCTION();

		if (!m_ImportSettings.PreserveHierarchy)
		{
			ReserveBuffers();
		}
		else
		{
			CreateSubMeshes();
		}

		VisitNode(m_Scene->mRootNode, glm::mat4(1.0f));
		WalkHierarchy(m_Scene->mRootNode, glm::mat4(1.0f));
	}

	inline static Math::AABB ComputeBounds(Span<const glm::vec3> vertices)
	{
		Math::AABB bounds{};
	
		if (vertices.GetSize() == 0)
			return bounds;

		bounds.Min = vertices[0];
		bounds.Max = vertices[0];

		for (size_t i = 0; i < vertices.GetSize(); i++)
		{
			bounds.Min = glm::min(bounds.Min, vertices[i]);
			bounds.Max = glm::max(bounds.Max, vertices[i]);
		}

		return bounds;
	}

	void StaticMeshImporter::CreateSubMeshes()
	{
		FLARE_PROFILE_FUNCTION();

		size_t vertexCount = 0;
		size_t indexCount = 0;

		CountMeshVerticesAndIndices(vertexCount, indexCount);
		
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

		for (uint32_t meshIndex = 0; meshIndex < m_Scene->mNumMeshes; meshIndex++)
		{
			const aiMesh* mesh = m_Scene->mMeshes[meshIndex];
			SubMesh subMesh = CopySubMeshData(mesh);

#if 0
			subMesh.Bounds.Min = glm::vec3(mesh->mAABB.mMin.x, mesh->mAABB.mMin.y, mesh->mAABB.mMin.z);
			subMesh.Bounds.Max = glm::vec3(mesh->mAABB.mMax.x, mesh->mAABB.mMax.y, mesh->mAABB.mMax.z);
#else
#endif

			m_SceneData.MeshData[mesh] = subMesh;
		}

		m_SceneData.SharedMesh = CreateRef<SharedMesh>(vertexCount, m_SceneData.IndexFormat, indexCount);

		Ref<CommandBuffer> commandBuffer = VulkanContext::GetInstance().GetUploadCommandBuffer();
		
		m_SceneData.SharedMesh->Vertices->SetData(MemorySpan::FromVector(m_SceneData.Vertices), 0, commandBuffer);
		m_SceneData.SharedMesh->Normals->SetData(MemorySpan::FromVector(m_SceneData.Normals), 0, commandBuffer);
		m_SceneData.SharedMesh->Tangents->SetData(MemorySpan::FromVector(m_SceneData.Tangents), 0, commandBuffer);
		m_SceneData.SharedMesh->UVs->SetData(MemorySpan::FromVector(m_SceneData.UVs), 0, commandBuffer);

		if (m_SceneData.IndexFormat == IndexBuffer::IndexFormat::UInt16)
			m_SceneData.SharedMesh->IndexBuffer->SetData(MemorySpan::FromVector(m_SceneData.Indices16), 0, commandBuffer);
		else
			m_SceneData.SharedMesh->IndexBuffer->SetData(MemorySpan::FromVector(m_SceneData.Indices32), 0, commandBuffer);
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

		if (!m_ImportSettings.PreserveHierarchy)
		{
			glm::mat4 nodeTransform = parentTransform * ConvertToColumnMajor(node->mTransformation);
			for (uint32_t i = 0; i < node->mNumMeshes; i++)
			{
				aiMesh* nodeMesh = m_Scene->mMeshes[node->mMeshes[i]];

				if (m_ImportSettings.ImportMaterials)
					m_SceneData.UsedMaterials.insert(nodeMesh->mMaterialIndex);

				size_t subMeshStart = m_VertexOffset;
				size_t subMeshEnd = m_VertexOffset + (size_t)nodeMesh->mNumVertices;

				SubMesh subMesh = CopySubMeshData(nodeMesh);
				FlattenHierarchy(node, nodeTransform, subMeshStart, subMeshEnd);

				subMesh.Bounds = ComputeBounds(Span(m_SceneData.Vertices.data() + subMeshStart, subMeshEnd - subMeshStart));

				m_SceneData.SubMeshes.push_back(subMesh);
			}
		}

		if (m_ImportSettings.PreserveHierarchy)
		{
			std::vector<SubMesh> subMeshes;

			NodeMesh& nodeMeshData = m_SceneData.NodeToMesh[node];
			for (uint32_t i = 0; i < node->mNumMeshes; i++)
			{
				aiMesh* nodeMesh = m_Scene->mMeshes[node->mMeshes[i]];

				nodeMeshData.MaterialIndices.push_back(nodeMesh->mMaterialIndex);
				m_SceneData.UsedMaterials.insert(nodeMesh->mMaterialIndex);
				subMeshes.push_back(m_SceneData.MeshData[nodeMesh]);
			}

			nodeMeshData.Mesh = CreateRef<Mesh>(m_SceneData.SharedMesh, std::move(subMeshes));
			nodeMeshData.Mesh->SetDebugName(node->mName.C_Str());
		}
	}

	SubMesh StaticMeshImporter::CopySubMeshData(const aiMesh* mesh)
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

		SubMesh subMesh{};
		subMesh.BaseVertex = 0;
		subMesh.BaseIndex = (uint32_t)m_IndexOffset;
		subMesh.IndicesCount = (uint32_t)subMeshIndexCount;
		subMesh.Bounds = ComputeBounds(Span(m_SceneData.Vertices.data() + m_VertexOffset, (size_t)mesh->mNumVertices));

		m_VertexOffset += mesh->mNumVertices;
		m_IndexOffset += subMeshIndexCount;

		return subMesh;
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

	void StaticMeshImporter::CountMeshVerticesAndIndices(size_t& outVertexCount, size_t& outIndexCount)
	{
		FLARE_PROFILE_FUNCTION();

		for (size_t i = 0; i < m_Scene->mNumMeshes; i++)
		{
			const aiMesh* mesh = m_Scene->mMeshes[i];

			outVertexCount += (size_t)mesh->mNumVertices;
			for (uint32_t face = 0; face < mesh->mNumFaces; face++)
			{
				outIndexCount += (size_t)mesh->mFaces[face].mNumIndices;
			}
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
