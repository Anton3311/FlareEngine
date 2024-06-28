#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include "Flare/DebugRenderer/DebugRendererFrameData.h"

namespace Flare
{
	class Shader;
	class Pipeline;
	class VertexBuffer;

	class DebugLinesPass : public RenderGraphPass
	{
	public:
		DebugLinesPass(Ref<Shader> debugShader,
			const DebugRendererSettings& settings,
			const DebugRendererFrameData& frameData);
		~DebugLinesPass();

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void CreatePipeline(const RenderGraphContext& context);
	private:
		const DebugRendererSettings& m_Settings;
		const DebugRendererFrameData& m_FrameData;

		Ref<VertexBuffer> m_VertexBuffer = nullptr;

		Ref<Shader> m_Shader = nullptr;
		Ref<Pipeline> m_Pipeline = nullptr;
	};
}
