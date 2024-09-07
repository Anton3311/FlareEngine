#include "DebugLinesPass.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/SceneSubmition.h"
#include "FLare/Renderer/Viewport.h"

#include "Flare/DebugRenderer/DebugRendererFrameData.h"

#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"

namespace Flare
{
	DebugLinesPass::DebugLinesPass(Ref<Shader> debugShader, const DebugRendererSettings& settings)
		: m_Shader(debugShader), m_Settings(settings)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			FrameResources& resources = m_FrameResources.emplace_back();
			resources.VertexBuffer = GPUBuffer::CreateVertexBuffer(sizeof(DebugRendererFrameData::Vertex) * 2 * m_Settings.MaxLines, GPUBufferMemoryType::Static);
		}
	}

	DebugLinesPass::~DebugLinesPass()
	{
	}

	void DebugLinesPass::OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		using Vertex = DebugRendererFrameData::Vertex;

		const FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
		const DebugRendererFrameData& submition = context.GetSceneSubmition().DebugRendererSubmition;

		frameResources.VertexBuffer->SetData(MemorySpan(submition.LineVertices.data(), submition.LineCount * 2), 0, commandBuffer);
	}

	void DebugLinesPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();

		const FrameResources& frameResources = m_FrameResources[GraphicsContext::GetInstance().GetCurrentFrameInFlight()];
		const DebugRendererFrameData& submition = context.GetSceneSubmition().DebugRendererSubmition;

		if (m_Pipeline == nullptr)
			CreatePipeline(context);

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = commandBuffer.As<VulkanCommandBuffer>();

		vulkanCommandBuffer->BindPipeline(m_Pipeline);
		vulkanCommandBuffer->BindVertexBuffers(Span((Ref<const GPUBuffer>*)&frameResources.VertexBuffer, 1), 0);
		vulkanCommandBuffer->BindDescriptorSet(context.GetViewport().GetFrameResources().CameraDescriptorSet, 0);

		vulkanCommandBuffer->Draw(0, (uint32_t)submition.LineCount * 2, 0, 1);
	}

	void DebugLinesPass::CreatePipeline(const RenderGraphContext& context)
	{
		FLARE_PROFILE_FUNCTION();
		PipelineSpecifications linePipelineSpecifications{};
		linePipelineSpecifications.Blending = BlendMode::Opaque;
		linePipelineSpecifications.Culling = CullingMode::None;
		linePipelineSpecifications.DepthFunction = DepthComparisonFunction::Less;
		linePipelineSpecifications.DepthTest = true;
		linePipelineSpecifications.DepthWrite = false;
		linePipelineSpecifications.Shader = m_Shader;
		linePipelineSpecifications.Topology = PrimitiveTopology::Lines;
		linePipelineSpecifications.InputLayout = PipelineInputLayout({
			{ 0, 0, ShaderDataType::Float3 },
			{ 0, 1, ShaderDataType::Float4 }
		});

		m_Pipeline = Pipeline::Create(linePipelineSpecifications);
	}
}
