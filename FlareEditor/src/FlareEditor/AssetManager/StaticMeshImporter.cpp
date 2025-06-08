#include "PCH.h"

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

	SceneData::~SceneData()
	{
		if (VertexDataBuffer)
			delete[] VertexDataBuffer;

		if (DepthOnlyVertices)
			delete[] DepthOnlyVertices;

		if (UVs)
			delete[] UVs;

		if (Indices16)
			delete[] Indices16;

		if (Indices32)
			delete[] Indices32;

		if (SubMeshes)
			delete[] SubMeshes;
	}

	StaticMeshImporter::~StaticMeshImporter()
	{
		if (m_TemporaryReducedIndexBuffer)
			delete[] m_TemporaryReducedIndexBuffer;

		if (m_TemporaryUniqueVertexMapping)
			delete[] m_TemporaryUniqueVertexMapping;
	}

	void StaticMeshImporter::Import()
	{
		FLARE_PROFILE_FUNCTION();

		size_t vertexCount = 0;
		size_t indexCount = 0;
		size_t subMeshCount = m_Scene->mNumMeshes;
		
		CountMeshVerticesAndIndices(vertexCount, indexCount);
		
		IndexFormat indexFormat = indexCount <= (size_t)std::numeric_limits<uint16_t>::max()
			? IndexFormat::UInt16
			: IndexFormat::UInt32;
		
		InitializeSceneData(vertexCount, indexCount, subMeshCount, indexFormat);

		if (m_ImportSettings.PreserveHierarchy)
		{
			for (uint32_t meshIndex = 0; meshIndex < m_Scene->mNumMeshes; meshIndex++)
			{
				const aiMesh* mesh = m_Scene->mMeshes[meshIndex];
				m_SceneData.MeshData[mesh] = CopySubMeshData(mesh);
			}
			
			CreateSharedMesh();
		}

		VisitNode(m_Scene->mRootNode, glm::mat4(1.0f));
		WalkHierarchy(m_Scene->mRootNode, glm::mat4(1.0f));
	}

	static Math::AABB ComputeBounds(Span<const glm::vec3> vertices)
	{
		FLARE_PROFILE_FUNCTION();
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

	void StaticMeshImporter::CreateSharedMesh()
	{
		FLARE_PROFILE_FUNCTION();
		m_SceneData.SharedMesh = Ref<SharedMesh>::New(m_SceneData.VertexCount,
			m_SceneData.DepthOnlyVertexCount,
			m_SceneData.IndexFormat,
			m_SceneData.IndexCount);

		Ref<CommandBuffer> commandBuffer = VulkanContext::GetInstance().GetUploadCommandBuffer();
		
		m_SceneData.SharedMesh->Vertices->SetData(MemorySpan(m_SceneData.Vertices, m_SceneData.VertexCount), 0, commandBuffer);
		m_SceneData.SharedMesh->Normals->SetData(MemorySpan(m_SceneData.Normals, m_SceneData.VertexCount), 0, commandBuffer);
		m_SceneData.SharedMesh->Tangents->SetData(MemorySpan(m_SceneData.Tangents, m_SceneData.VertexCount), 0, commandBuffer);
		m_SceneData.SharedMesh->UVs->SetData(MemorySpan(m_SceneData.UVs, m_SceneData.VertexCount), 0, commandBuffer);

		m_SceneData.SharedMesh->DepthOnlyVertices->SetDebugName("DepthOnly Vertices");

		m_SceneData.SharedMesh->DepthOnlyVertices->SetData(
			MemorySpan(m_SceneData.DepthOnlyVertices, m_SceneData.DepthOnlyVertexCount),
			0, commandBuffer);

		if (m_SceneData.IndexFormat == IndexFormat::UInt16)
		{
			m_SceneData.SharedMesh->IndexBuffer->SetData(MemorySpan(m_SceneData.Indices16, m_SceneData.IndexCount),
					0, commandBuffer);
			m_SceneData.SharedMesh->DepthOnlyIndexBuffer->SetData(MemorySpan(m_SceneData.DepthOnlyIndices16, m_SceneData.IndexCount),
					0, commandBuffer);
		}
		else
		{
			m_SceneData.SharedMesh->IndexBuffer->SetData(MemorySpan(m_SceneData.Indices32, m_SceneData.IndexCount),
					0, commandBuffer);
			m_SceneData.SharedMesh->DepthOnlyIndexBuffer->SetData(MemorySpan(m_SceneData.DepthOnlyIndices32, m_SceneData.IndexCount),
					0, commandBuffer);
		}
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

				auto [subMesh, depthOnlySubMesh] = CopySubMeshData(nodeMesh);
				FlattenHierarchy(node, nodeTransform, subMeshStart, subMeshEnd);

				subMesh.Bounds = ComputeBounds(Span(m_SceneData.Vertices + subMeshStart, subMeshEnd - subMeshStart));
				depthOnlySubMesh.Bounds = subMesh.Bounds;

				m_SceneData.SubMeshes[m_InsertedSubMeshCount] = subMesh;
				m_SceneData.DepthOnlySubMeshes[m_InsertedSubMeshCount] = depthOnlySubMesh;
				m_InsertedSubMeshCount++;
			}
		}

		if (m_ImportSettings.PreserveHierarchy)
		{
			size_t previousSubMeshCount = m_InsertedSubMeshCount;
			std::vector<SubMesh> subMeshes;
			std::vector<SubMesh> depthOnlySubMeshes;

			NodeMesh& nodeMeshData = m_SceneData.NodeToMesh[node];
			for (uint32_t i = 0; i < node->mNumMeshes; i++)
			{
				aiMesh* nodeMesh = m_Scene->mMeshes[node->mMeshes[i]];

				nodeMeshData.MaterialIndices.push_back(nodeMesh->mMaterialIndex);
				m_SceneData.UsedMaterials.insert(nodeMesh->mMaterialIndex);

				auto [subMesh, depthOnlySubMesh] = m_SceneData.MeshData[nodeMesh];
				m_SceneData.SubMeshes[m_InsertedSubMeshCount] = subMesh;
				m_SceneData.DepthOnlySubMeshes[m_InsertedSubMeshCount] = depthOnlySubMesh;
				m_InsertedSubMeshCount++;
			}

			size_t subMeshCount = static_cast<size_t>(node->mNumMeshes);
			nodeMeshData.Mesh = Ref<Mesh>::New(m_SceneData.SharedMesh,
					std::vector<SubMesh>(m_SceneData.SubMeshes + previousSubMeshCount, m_SceneData.SubMeshes + m_InsertedSubMeshCount),
					std::vector<SubMesh>(m_SceneData.DepthOnlySubMeshes + previousSubMeshCount, m_SceneData.DepthOnlySubMeshes + m_InsertedSubMeshCount));
			nodeMeshData.Mesh->SetDebugName(node->mName.C_Str());
		}
	}

	struct Vector3Hasher
	{
	public:
		size_t operator()(const glm::vec3& vector) const
		{
			size_t hash = 0;
			CombineHashes(hash, vector.x);
			CombineHashes(hash, vector.y);
			CombineHashes(hash, vector.z);
			return hash;
		}
	};

	template<typename IndexType>
	using VertexToIndexMapping = std::unordered_map<glm::vec3, IndexType, Vector3Hasher>;

	template<typename IndexType>
	static void GenerateDepthOnlyIndices(Span<const glm::vec3> vertices,
		Span<const IndexType> indices,
		IndexType* outputBuffer,
		size_t& uniqueVertexCount)
	{
		FLARE_PROFILE_FUNCTION();

		VertexToIndexMapping<IndexType> vertexToIndex;
		vertexToIndex.reserve(indices.GetSize());

		for (IndexType index : indices)
		{
			vertexToIndex.try_emplace(vertices[index], index);
		}

		for (size_t i = 0; i < indices.GetSize(); i++)
		{
			outputBuffer[i] = vertexToIndex[vertices[indices[i]]];
		}

		uniqueVertexCount = vertexToIndex.size();
	}

	template<typename IndexType>
	static void GenerateUniqueVertices(Span<const glm::vec3>& vertices,
		Span<const IndexType> depthOnlyIndices,
		Span<glm::vec3> reducedVertexBuffer,
		Span<IndexType> reducedIndexBuffer,
		Span<IndexType> indexMapping)
	{
		FLARE_PROFILE_FUNCTION();

		constexpr IndexType INVALID_INDEX = std::numeric_limits<IndexType>::max();

		std::memset(indexMapping.GetData(), 0xff, indexMapping.GetSize() * sizeof(IndexType));

		IndexType reducedVertexBufferOffset = 0;
		IndexType reducedIndexBufferOffset = 0;
		for (IndexType index : depthOnlyIndices)
		{
			if (indexMapping[index] == INVALID_INDEX)
			{
				reducedVertexBuffer[reducedVertexBufferOffset] = vertices[index];
				indexMapping[index] = reducedVertexBufferOffset;
				reducedVertexBufferOffset++;
			}

			reducedIndexBuffer[reducedIndexBufferOffset] = indexMapping[index];

			reducedIndexBufferOffset++;
		}
	}

	SubMeshesPair StaticMeshImporter::CopySubMeshData(const aiMesh* mesh)
	{
		FLARE_PROFILE_FUNCTION();

		std::memcpy(m_SceneData.Vertices + m_VertexOffset, mesh->mVertices, sizeof(glm::vec3) * mesh->mNumVertices);
		std::memcpy(m_SceneData.Normals + m_VertexOffset, mesh->mNormals, sizeof(glm::vec3) * mesh->mNumVertices);
		std::memcpy(m_SceneData.Tangents + m_VertexOffset, mesh->mTangents, sizeof(glm::vec3) * mesh->mNumVertices);

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
		if (m_SceneData.IndexFormat == IndexFormat::UInt16)
		{
			uint16_t* indexWriteLocation = m_SceneData.Indices16 + m_IndexOffset;
			for (uint32_t face = 0; face < mesh->mNumFaces; face++)
			{
				const aiFace& f = mesh->mFaces[face];
				subMeshIndexCount += f.mNumIndices;

				for (uint32_t i = 0; i < f.mNumIndices; i++)
				{
					*indexWriteLocation = static_cast<uint16_t>(f.mIndices[i]) + static_cast<uint16_t>(m_VertexOffset);
					indexWriteLocation++;
				}
			}
		}
		else
		{
			uint32_t* indexWriteLocation = m_SceneData.Indices32 + m_IndexOffset;
			for (uint32_t face = 0; face < mesh->mNumFaces; face++)
			{
				const aiFace& f = mesh->mFaces[face];
				subMeshIndexCount += f.mNumIndices;

				for (uint32_t i = 0; i < f.mNumIndices; i++)
				{
					*indexWriteLocation = static_cast<uint32_t>(f.mIndices[i]) + static_cast<uint32_t>(m_VertexOffset);
					indexWriteLocation++;
				}
			}
		}

		SubMesh subMesh{};
		subMesh.BaseVertex = 0;
		subMesh.BaseIndex = (uint32_t)m_IndexOffset;
		subMesh.IndicesCount = (uint32_t)subMeshIndexCount;
		subMesh.Bounds = ComputeBounds(Span(m_SceneData.Vertices + m_VertexOffset, (size_t)mesh->mNumVertices));

		SubMesh depthOnlySubMesh{};
		depthOnlySubMesh.BaseVertex = 0;
		depthOnlySubMesh.Bounds = subMesh.Bounds;
		// NOTE: Each submesh has the same total number of regular and depthonly indices, so the offsets and counts match
		depthOnlySubMesh.BaseIndex = subMesh.BaseIndex;
		depthOnlySubMesh.IndicesCount = subMesh.IndicesCount;

		size_t uniqueVertexCount = 0;
		Span<const glm::vec3> allVertices = Span(m_SceneData.Vertices, m_SceneData.VertexCount);

		FLARE_CORE_ASSERT(subMeshIndexCount <= m_MaxSubMeshIndexCount);
		if (m_SceneData.IndexFormat == IndexFormat::UInt16)
		{
			uint16_t* reducedIndexBuffer = reinterpret_cast<uint16_t*>(m_TemporaryReducedIndexBuffer);
			GenerateDepthOnlyIndices<uint16_t>(allVertices,
				Span(m_SceneData.Indices16 + m_IndexOffset, subMeshIndexCount),
				reducedIndexBuffer,
				uniqueVertexCount);
			
			GenerateUniqueVertices<uint16_t>(allVertices,
				Span(reducedIndexBuffer, subMeshIndexCount),
				Span(m_SceneData.DepthOnlyVertices + m_SceneData.DepthOnlyVertexCount, uniqueVertexCount),
				Span(m_SceneData.DepthOnlyIndices16 + m_IndexOffset, subMeshIndexCount),
				Span(reinterpret_cast<uint16_t*>(m_TemporaryUniqueVertexMapping), m_SceneData.VertexCount));
		}
		else
		{
			GenerateDepthOnlyIndices<uint32_t>(allVertices,
				Span(m_SceneData.Indices32 + m_IndexOffset, subMeshIndexCount),
				m_TemporaryReducedIndexBuffer,
				uniqueVertexCount);

			GenerateUniqueVertices<uint32_t>(allVertices,
				Span(m_TemporaryReducedIndexBuffer, subMeshIndexCount),
				Span(m_SceneData.DepthOnlyVertices + m_SceneData.DepthOnlyVertexCount, uniqueVertexCount),
				Span(m_SceneData.DepthOnlyIndices32 + m_IndexOffset, subMeshIndexCount),
				Span(m_TemporaryUniqueVertexMapping, m_SceneData.VertexCount));
		}

		depthOnlySubMesh.BaseVertex = static_cast<uint32_t>(m_SceneData.DepthOnlyVertexCount);
		m_SceneData.DepthOnlyVertexCount += uniqueVertexCount;

		m_VertexOffset += mesh->mNumVertices;
		m_IndexOffset += subMeshIndexCount;

		return { subMesh, depthOnlySubMesh };
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

	void StaticMeshImporter::InitializeSceneData(size_t vertexCount, size_t indexCount, size_t subMeshCount, IndexFormat indexFormat)
	{
		FLARE_PROFILE_FUNCTION();

		m_SceneData.VertexCount = vertexCount;
		m_SceneData.IndexCount = indexCount;
		m_SceneData.IndexFormat = indexFormat;
		m_SceneData.SubMeshCount = subMeshCount;

		m_SceneData.DepthOnlyVertices = new glm::vec3[vertexCount];

		constexpr size_t VECTOR3_VERTEX_ATTRIBUTE_COUNT = 3;
		size_t vertexDataBufferSize = vertexCount * VECTOR3_VERTEX_ATTRIBUTE_COUNT;
		m_SceneData.VertexDataBuffer = new glm::vec3[vertexDataBufferSize];

		m_SceneData.Vertices = m_SceneData.VertexDataBuffer;
		m_SceneData.Normals = m_SceneData.VertexDataBuffer + vertexCount;
		m_SceneData.Tangents = m_SceneData.VertexDataBuffer + vertexCount * 2;
		m_SceneData.UVs = new glm::vec2[vertexCount];

		m_SceneData.SubMeshes = new SubMesh[subMeshCount * 2];
		m_SceneData.DepthOnlySubMeshes = m_SceneData.SubMeshes + subMeshCount;

		m_TemporaryReducedIndexBuffer = new uint32_t[m_MaxSubMeshIndexCount];
		m_TemporaryUniqueVertexMapping = new uint32_t[m_SceneData.VertexCount];

		switch (indexFormat)
		{
		case IndexFormat::UInt16:
		{
			uint16_t* indices = new uint16_t[m_SceneData.IndexCount * 2];
			m_SceneData.Indices16 = indices;
			m_SceneData.DepthOnlyIndices16 = indices + m_SceneData.IndexCount;
			break;
		}
		case IndexFormat::UInt32:
		{
			uint32_t* indices = new uint32_t[m_SceneData.IndexCount * 2];
			m_SceneData.Indices32 = indices;
			m_SceneData.DepthOnlyIndices32 = indices + m_SceneData.IndexCount;
			break;
		}
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

			size_t meshIndexCount = 0;

			for (uint32_t face = 0; face < mesh->mNumFaces; face++)
			{
				meshIndexCount += (size_t)mesh->mFaces[face].mNumIndices;
			}

			outVertexCount += (size_t)mesh->mNumVertices;
			outIndexCount += meshIndexCount;

			m_MaxSubMeshIndexCount = glm::max(m_MaxSubMeshIndexCount, meshIndexCount);
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

		m_MaxSubMeshIndexCount = glm::max(m_MaxSubMeshIndexCount, indexCount);
	}
}
