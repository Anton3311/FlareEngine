#pragma once

#include "Flare/AssetManager/Asset.h"
#include "Flare/Renderer/VertexArray.h"
#include "Flare/Renderer/Buffer.h"
#include "Flare/Math/Math.h"

#include "FlareCore/Collections/Span.h"

namespace Flare
{
	struct SubMesh
	{
		Math::AABB Bounds;

		Ref<VertexArray> VertexArray = nullptr;
		Ref<IndexBuffer> Indicies = nullptr;
		Ref<VertexBuffer> Vertices = nullptr;
		Ref<VertexBuffer> Normals = nullptr;
		Ref<VertexBuffer> UVs = nullptr;
	};

	class FLARE_API Mesh : public Asset
	{
	public:
		Mesh();

		void AddSubMesh(const Span<glm::vec3>& vertices,
			const Span<uint32_t>& indices,
			const Span<glm::vec3>* normals = nullptr,
			const Span<glm::vec2>* uvs = nullptr);

		inline Ref<VertexBuffer> GetInstanceBuffer() const { return m_InstanceBuffer; }
		void SetInstanceBuffer(const Ref<VertexBuffer>& instanceBuffer);

		inline const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
	private:
		void InitializeLastSubMeshBuffers();
	private:
		Ref<VertexBuffer> m_InstanceBuffer = nullptr;
		std::vector<SubMesh> m_SubMeshes;
	};
}