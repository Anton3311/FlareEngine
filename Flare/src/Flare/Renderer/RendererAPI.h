#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/VertexArray.h"

#include <stdint.h>

namespace Flare
{
	enum class CullingMode : uint8_t
	{
		None,
		Back,
		Front,
	};

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

		virtual void SetDepthTestEnabled(bool enabled) = 0;
		virtual void SetCullingMode(CullingMode mode) = 0;

		virtual void SetLineWidth(float width) = 0;

		virtual void DrawIndexed(const Ref<const VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const Ref<const VertexArray>& vertexArray, size_t indicesCount) = 0;
		virtual void DrawLines(const Ref<const VertexArray>& vertexArray, size_t cverticesCountount) = 0;
	public:
		static Scope<RendererAPI> Create();
		static API GetAPI();
	};
}