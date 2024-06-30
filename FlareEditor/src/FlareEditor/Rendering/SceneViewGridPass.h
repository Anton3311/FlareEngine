#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	class VertexBuffer;
	class Shader;
	class Pipeline;
	class SceneViewGridPass : public RenderGraphPass
	{
	public:
		SceneViewGridPass();

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void GenerateGridMesh();
		void CreatePipeline(const RenderGraphContext& context);
	private:
		Ref<Shader> m_Shader = nullptr;
		Ref<Pipeline> m_Pipeline = nullptr;
		Ref<VertexBuffer> m_VertexBuffer = nullptr;

		uint32_t m_CellCount = 0;
		uint32_t m_VertexCount = 0;
	};
}
