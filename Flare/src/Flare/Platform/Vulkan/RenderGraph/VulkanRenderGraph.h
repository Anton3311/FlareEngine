#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

namespace Flare
{
	class FLARE_API VulkanRenderGraph : public RenderGraph
	{
	public:
		VulkanRenderGraph(const Viewport& viewport);
	protected:
		virtual void OnPrepare() {}
		virtual void OnTexturesResize() {}
		virtual void OnClear() {}
		virtual void OnBuild() {}
	private:
	};
}
