#pragma once

#include <Flare/Renderer/Buffer.h>
#include <Flare/Renderer/VertexArray.h>
#include <Flare/Renderer/Shader.h>

#include <glm/glm.hpp>

#include <vector>

namespace Flare
{
	struct QuadVertex
	{
		glm::vec2 Position;
		glm::vec4 Color;
	};

	struct Renderer2DData
	{
		size_t QuadIndex;
		size_t MaxQuadCount;

		std::vector<QuadVertex> Vertices;

		Ref<VertexArray> VertexArray;
		Ref<VertexBuffer> VertexBuffer;
		Ref<IndexBuffer> IndexBuffer;

		Ref<Shader> CurrentShader;

		glm::mat4 CameraProjectionMatrix;
	};

	class Renderer2D
	{
	public:
		static void Initialize(size_t maxQuads = 10000);
		static void Shutdown();

		static void Begin(const Ref<Shader>& shader, const glm::mat4& projectionMatrix);
		static void Flush();
		static void Submit(glm::vec2 position, glm::vec2 size, glm::vec4 color);
		static void End();
	private:
		static Renderer2DData* s_Data;
	};
}
