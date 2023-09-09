#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RendererAPI.h"

namespace Flare
{
	class FLARE_API RenderCommand
	{
	public:
		static void Initialize();
		static void Clear();
		static void SetLineWidth(float width);
		static void SetClearColor(float r, float g, float b, float a);
		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
		static void DrawIndexed(const Ref<VertexArray>& mesh);
		static void DrawIndexed(const Ref<VertexArray>& mesh, size_t indicesCount);
		static void DrawLines(const Ref<VertexArray>& lines, size_t verticesCount);
	};
}