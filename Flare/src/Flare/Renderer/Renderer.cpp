#include "Renderer.h"

namespace Flare
{
	struct RendererData
	{
		glm::uvec2 ViewportSize;
	};

	RendererData s_RendererData;

	void Renderer::SetMainViewportSize(glm::uvec2 size)
	{
		s_RendererData.ViewportSize = size;
	}

	glm::uvec2 Renderer::GetMainViewportSize()
	{
		return s_RendererData.ViewportSize;
	}
}
