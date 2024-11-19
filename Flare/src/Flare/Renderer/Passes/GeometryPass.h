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
		GeometryPass(RendererStatistics& statistics, Ref<Material> materialOverride);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		std::optional<float> GetElapsedTime() const;
	private:
		struct Batch
		{
			Ref<const Mesh> Mesh = nullptr;
			Ref<const Material> Material = nullptr;
			uint32_t SubMesh = 0;
			uint32_t BaseInstance = 0;
			uint32_t InstanceCount = 0;
		};

		void FlushBatch(const Ref<CommandBuffer>& commandBuffer, const Batch& batch);
	private:
		Ref<GPUTimer> m_Timer = nullptr;
		Ref<Material> m_MaterialOverride = nullptr;

		RendererStatistics& m_Statistics;
	};
}
