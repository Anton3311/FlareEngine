#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include "Flare/DebugRenderer/DebugRendererFrameData.h"

#include "Flare/Renderer/Pipeline.h"

namespace Flare
{
	class IndexBuffer;
	class VertexBuffer;
	class Shader;
	class DebugRaysPass : public RenderGraphPass
	{
	public:
		DebugRaysPass(Ref<IndexBuffer> indexBuffer, Ref<Shader> debugShader, const DebugRendererFrameData& frameData, const DebugRendererSettings& settings);

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void CreatePipeline(const RenderGraphContext& context);
		void GenerateVertices(const RenderGraphContext& context);
	private:
		const DebugRendererFrameData& m_FrameData;
		const DebugRendererSettings& m_Settings;

		Ref<Shader> m_Shader = nullptr;
		Ref<Pipeline> m_Pipeline = nullptr;

		Ref<VertexBuffer> m_VertexBuffer = nullptr;
		Ref<IndexBuffer> m_IndexBuffer = nullptr;

		std::vector<DebugRendererFrameData::Vertex> m_Vertices;
	};
}
