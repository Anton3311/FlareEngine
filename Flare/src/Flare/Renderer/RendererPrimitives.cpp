#include "RendererPrimitives.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Mesh.h"

namespace Flare
{
	struct RendererPrimitivesData
	{
		Ref<Mesh> Cube = nullptr;
		Ref<Mesh> FullscreenQuadMesh = nullptr;
	};

	static RendererPrimitivesData s_Primitives;

	void RendererPrimitives::Initialize()
	{
		FLARE_PROFILE_FUNCTION();

		GetCube();
		GetFullscreenQuadMesh();
	}

	Ref<const Mesh> RendererPrimitives::GetCube()
	{
		FLARE_PROFILE_FUNCTION();
		if (s_Primitives.Cube)
			return s_Primitives.Cube;

		glm::vec3 cubeVertices[] =
		{
			glm::vec3(-0.5f, -0.5f, -0.5f),
			glm::vec3(+0.5f, -0.5f, -0.5f),
			glm::vec3(+0.5f, +0.5f, -0.5f),
			glm::vec3(-0.5f, +0.5f, -0.5f),

			glm::vec3(-0.5f, -0.5f, +0.5f),
			glm::vec3(+0.5f, -0.5f, +0.5f),
			glm::vec3(+0.5f, +0.5f, +0.5f),
			glm::vec3(-0.5f, +0.5f, +0.5f),
		};

		glm::vec3 cubeNormals[] =
		{
			glm::vec3(0.0f),
			glm::vec3(0.0f),
			glm::vec3(0.0f),
			glm::vec3(0.0f),

			glm::vec3(0.0f),
			glm::vec3(0.0f),
			glm::vec3(0.0f),
			glm::vec3(0.0f),
		};

		glm::vec2 cubeUVs[] =
		{
			glm::vec2(0.0f),
			glm::vec2(0.0f),
			glm::vec2(0.0f),
			glm::vec2(0.0f),

			glm::vec2(0.0f),
			glm::vec2(0.0f),
			glm::vec2(0.0f),
			glm::vec2(0.0f),
		};

		uint16_t cubeIndices[] =
		{
			// Front
			1, 2, 0,
			2, 3, 0,

			// Left
			3, 4, 0,
			4, 3, 7,

			// Right
			6, 2, 1,
			6, 1, 5,

			// Back
			6, 5, 4,
			7, 6, 4,

			// Bottom
			0, 4, 1,
			1, 4, 5,

			// Top
			7, 3, 2,
			6, 7, 2
		};

		s_Primitives.Cube = Mesh::Create(MemorySpan(cubeIndices, sizeof(cubeIndices) / sizeof(cubeIndices[0])),
			IndexFormat::UInt16,
			Span(cubeVertices, 8),
			Span(cubeNormals, 8),
			Span(cubeNormals, 8),
			Span(cubeUVs, 8));

		return s_Primitives.Cube;
	}

	Ref<const Mesh> RendererPrimitives::GetFullscreenQuadMesh()
	{
		FLARE_PROFILE_FUNCTION();
		if (s_Primitives.FullscreenQuadMesh)
			return s_Primitives.FullscreenQuadMesh;

		glm::vec3 vertices[] =
		{
			{ -1.0f, -1.0f, 0.0f },
			{ -1.0f,  1.0f, 0.0f },
			{  1.0f,  1.0f, 0.0f },
			{  1.0f, -1.0f, 0.0f },
		};

		glm::vec3 normals[]
		{
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f },
			{ 0.0f, 0.0f, 0.0f },
		};

		glm::vec2 uvs[] =
		{
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
			{ 0.0f, 0.0f },
		};

		uint16_t indices[] =
		{
			0, 1, 2,
			0, 2, 3,
		};

		s_Primitives.FullscreenQuadMesh = Mesh::Create(MemorySpan(indices, 6),
			IndexFormat::UInt16,
			Span(vertices, 4),
			Span(normals, 4),
			Span(normals, 4),
			Span(uvs, 4));		

		return s_Primitives.FullscreenQuadMesh;
	}

	void RendererPrimitives::Clear()
	{
		s_Primitives = {};
	}
}
