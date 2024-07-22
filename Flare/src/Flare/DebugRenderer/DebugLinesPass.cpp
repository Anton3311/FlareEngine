#include "DebugLinesPass.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/SceneSubmition.h"
#include "FLare/Renderer/Viewport.h"

#include "Flare/DebugRenderer/DebugRendererFrameData.h"

#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"

namespace Flare
{
	DebugLinesPass::DebugLinesPass(Ref<Shader> debugShader, const DebugRendererSettings& settings)
		: m_Shader(debugShader), m_Settings(settings)
	{
		FLARE_PROFILE_FUNCTION();
		m_VertexBuffer = VertexBuffer::Create(sizeof(DebugRendererFrameData::Vertex) * 2 * m_Settings.MaxLines, GPUBufferUsage::Static);
		As<VulkanVertexBuffer>(m_VertexBuffer)->GetBuffer().EnsureAllocated(); // HACk: To avoid binding NULL buffer
	}

	DebugLinesPass::~DebugLinesPass()
	{
	}

	void DebugLinesPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		using Vertex = DebugRendererFrameData::Vertex;

		const DebugRendererFrameData& submition = context.GetSceneSubmition().DebugRendererSubmition;

		m_VertexBuffer->SetData(MemorySpan(submition.LineVertices.data(), submition.LineCount * 2), 0, commandBuffer);

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		if (m_Pipeline == nullptr)
			CreatePipeline(context);

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);

		vulkanCommandBuffer->BindPipeline(m_Pipeline);
		vulkanCommandBuffer->BindVertexBuffers(Span((Ref<const VertexBuffer>*)&m_VertexBuffer, 1), 0);
		vulkanCommandBuffer->BindDescriptorSet(
			As<VulkanDescriptorSet>(context.GetViewport().GlobalResources.CameraDescriptorSet),
			As<VulkanPipeline>(m_Pipeline)->GetLayoutHandle(), 0);

		vulkanCommandBuffer->Draw(0, (uint32_t)submition.LineCount * 2, 0, 1);

		commandBuffer->EndRenderTarget();
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

		Ref<VulkanFrameBuffer> vulkanFrameBuffer = As<VulkanFrameBuffer>(context.GetRenderTarget());

		m_Pipeline = CreateRef<VulkanPipeline>(
			linePipelineSpecifications,
			vulkanFrameBuffer->GetCompatibleRenderPass());
	}
}
