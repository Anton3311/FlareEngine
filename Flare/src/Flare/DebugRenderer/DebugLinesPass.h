#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

namespace Flare
{
	struct DebugRendererSettings;

	class Shader;
	class Pipeline;
	class VertexBuffer;

	class DebugLinesPass : public RenderGraphPass
	{
	public:
		DebugLinesPass(Ref<Shader> debugShader, const DebugRendererSettings& settings);
		~DebugLinesPass();

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void CreatePipeline(const RenderGraphContext& context);
	private:
		struct FrameResources
		{
			Ref<VertexBuffer> VertexBuffer = nullptr;
		};

		const DebugRendererSettings& m_Settings;
		std::vector<FrameResources> m_FrameResources;

		Ref<Shader> m_Shader = nullptr;
		Ref<Pipeline> m_Pipeline = nullptr;
	};
}
