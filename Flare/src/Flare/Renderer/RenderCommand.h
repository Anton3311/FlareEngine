#pragma once

#include "Flare/Renderer/RendererAPI.h"

namespace Flare
{
	class RenderCommand
	{
	public:
		static void Initialize()
		{
			s_API->Initialize();
		}

		static void Clear()
		{
			s_API->Clear();
		}

		static void SetClearColor(float r, float g, float b, float a)
		{
			s_API->SetClearColor(r, g, b, a);
		}

		static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
		{
			s_API->SetViewport(x, y, width, height);
		}

		static void DrawIndexed(const Ref<VertexArray>& mesh)
		{
			s_API->DrawIndexed(mesh);
		}

		static void DrawIndexed(const Ref<VertexArray>& mesh, size_t indicesCount)
		{
			s_API->DrawIndexed(mesh, indicesCount);
		}
	private:
		static Scope<RendererAPI> s_API;
	};
}