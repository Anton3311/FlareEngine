#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/VertexArray.h"

#include <stdint.h>

namespace Flare
{
	class FLARE_API RendererAPI
	{
	public:
		enum class API
		{
			None,
			OpenGL,
		};
	public:
		virtual void Initialize() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

		virtual void SetClearColor(float r, float g, float b, float a) = 0;
		virtual void Clear() = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, size_t indicesCount) = 0;
	public:
		static Scope<RendererAPI> Create();
		static API GetAPI();
	};
}