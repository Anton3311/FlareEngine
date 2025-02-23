#pragma once

#include "Flare/Renderer/RendererStatistics.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include <vector>

namespace Flare
{
	class GPUTimer;
	class Material;
	class Mesh;

	class GeometryPass : public RenderGraphPass 
	{
	public:
		GeometryPass(RendererStatistics& statistics, Ref<Material> materialOverride, bool isDepthOnly);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		bool m_IsDepthOnly = false;
		Ref<Material> m_MaterialOverride = nullptr;

		RendererStatistics& m_Statistics;
	};
}
