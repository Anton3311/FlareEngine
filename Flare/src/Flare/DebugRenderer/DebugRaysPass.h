#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include "Flare/DebugRenderer/DebugRendererFrameData.h"

#include "Flare/Renderer/Pipeline.h"

namespace Flare
{
	class GPUBuffer;
	class Shader;
	class DebugRaysPass : public RenderGraphPass
	{
	public:
		DebugRaysPass(Ref<GPUBuffer> indexBuffer, Ref<Shader> debugShader, const DebugRendererSettings& settings);

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void CreatePipeline(const RenderGraphContext& context);
		void GenerateVertices(const RenderGraphContext& context);
	private:
		struct FrameResources
		{
			Ref<GPUBuffer> VertexBuffer = nullptr;
		};

		const DebugRendererSettings& m_Settings;

		Ref<Shader> m_Shader = nullptr;
		Ref<Pipeline> m_Pipeline = nullptr;

		Ref<GPUBuffer> m_IndexBuffer = nullptr;

		std::vector<FrameResources> m_FrameResources;
		std::vector<DebugRendererFrameData::Vertex> m_Vertices;
	};
}
