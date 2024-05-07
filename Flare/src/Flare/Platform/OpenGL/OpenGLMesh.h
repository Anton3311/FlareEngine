#pragma once

#include "Flare/Renderer/Mesh.h"
#include "Flare/Platform/OpenGL/OpenGLVertexArray.h"

namespace Flare
{
	class OpenGLMesh : public Mesh
	{
	public:
		OpenGLMesh(MeshTopology topologyType,
			size_t vertexBufferSize,
			IndexBuffer::IndexFormat indexFormat,
			size_t indexBufferSize);

		OpenGLMesh(MeshTopology topology,
			MemorySpan indices,
			IndexBuffer::IndexFormat indexFormat,
			Span<glm::vec3> vertices,
			Span<glm::vec3> normals,
			Span<glm::vec3> tangents,
			Span<glm::vec2> uvs);

		void AddSubMesh(const Span<glm::vec3>& vertices,
			const MemorySpan& indices,
			const Span<glm::vec3>& normals,
			const Span<glm::vec3>& tangents,
			const Span<glm::vec2>& uvs) override;

		inline const OpenGLVertexArray& GetVertexArray() const { return m_VertexArray; }
	private:
		void CreateVertexArray();
	private:
		OpenGLVertexArray m_VertexArray;
	};
}
