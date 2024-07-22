#include "DebugRaysPass.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/RenderData.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/SceneSubmition.h"
#include "Flare/Renderer/Viewport.h"

#include "Flare/Platform/Vulkan/VulkanFrameBuffer.h"
#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanVertexBuffer.h"

namespace Flare
{
	DebugRaysPass::DebugRaysPass(Ref<IndexBuffer> indexBuffer, Ref<Shader> debugShader, const DebugRendererSettings& settings)
		: m_Shader(debugShader), m_Settings(settings), m_IndexBuffer(indexBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		size_t bufferSize = sizeof(DebugRendererFrameData::Vertex) * DebugRendererSettings::VerticesPerRay * m_Settings.MaxRays;
		m_VertexBuffer = VertexBuffer::Create(bufferSize, GPUBufferUsage::Static);
		As<VulkanVertexBuffer>(m_VertexBuffer)->GetBuffer().EnsureAllocated(); // HACk: To avoid binding NULL buffer
	}

	void DebugRaysPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		GenerateVertices(context);

		const DebugRendererFrameData& submition = context.GetSceneSubmition().DebugRendererSubmition;

		using Vertex = DebugRendererFrameData::Vertex;
		m_VertexBuffer->SetData(
			MemorySpan(const_cast<Vertex*>(m_Vertices.data()), m_Vertices.size()),
			0, commandBuffer);

		if (m_Pipeline == nullptr)
			CreatePipeline(context);

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
		vulkanCommandBuffer->BindPipeline(m_Pipeline);
		vulkanCommandBuffer->BindVertexBuffers(Span((Ref<const VertexBuffer>*)&m_VertexBuffer, 1), 0);
		vulkanCommandBuffer->BindIndexBuffer(m_IndexBuffer);

		vulkanCommandBuffer->BindDescriptorSet(
			As<VulkanDescriptorSet>(context.GetViewport().GlobalResources.CameraDescriptorSet),
			As<VulkanPipeline>(m_Pipeline)->GetLayoutHandle(),
			0);

		vulkanCommandBuffer->DrawIndexed(0, (uint32_t)submition.RayCount * DebugRendererSettings::IndicesPerRay, 0, 0, 1);

		commandBuffer->EndRenderTarget();
	}

	void DebugRaysPass::CreatePipeline(const RenderGraphContext& context)
	{
		FLARE_PROFILE_FUNCTION();
		PipelineSpecifications rayPipelineSpecifications{};
		rayPipelineSpecifications.Blending = BlendMode::Opaque;
		rayPipelineSpecifications.Culling = CullingMode::None;
		rayPipelineSpecifications.DepthFunction = DepthComparisonFunction::Less;
		rayPipelineSpecifications.DepthTest = true;
		rayPipelineSpecifications.DepthWrite = false;
		rayPipelineSpecifications.Shader = m_Shader;
		rayPipelineSpecifications.Topology = PrimitiveTopology::Triangles;
		rayPipelineSpecifications.InputLayout = PipelineInputLayout({
			{ 0, 0, ShaderDataType::Float3 },
			{ 0, 1, ShaderDataType::Float4 }
		});

		Ref<VulkanFrameBuffer> vulkanFrameBuffer = As<VulkanFrameBuffer>(context.GetRenderTarget());

		m_Pipeline = CreateRef<VulkanPipeline>(
			rayPipelineSpecifications,
			vulkanFrameBuffer->GetCompatibleRenderPass());
	}

	void DebugRaysPass::GenerateVertices(const RenderGraphContext& context)
	{
		FLARE_PROFILE_FUNCTION();
		const Viewport& viewport = context.GetViewport();
		const float triangleHeight = glm::sqrt(3.0f) * m_Settings.RayThickness;

		const DebugRendererFrameData& submition = context.GetSceneSubmition().DebugRendererSubmition;

		uint32_t totalVertexCount = submition.RayCount * DebugRendererSettings::VerticesPerRay;

		if (totalVertexCount > m_Vertices.size())
			m_Vertices.resize(totalVertexCount);

		const RenderView& renderView = context.GetRenderView();
		for (uint32_t rayIndex = 0; rayIndex < submition.RayCount; rayIndex++)
		{
			const DebugRayData& ray = submition.Rays[rayIndex];

			glm::vec3 fromCameraDirection = renderView.Position - ray.Origin;
			glm::vec3 up = glm::normalize(glm::cross(fromCameraDirection, ray.Direction)) * m_Settings.RayThickness / 2.0f;

			for (uint32_t i = 0; i < DebugRendererSettings::VerticesPerRay; i++)
				m_Vertices[i].Color = ray.Color;

			glm::vec3 end = ray.Origin + ray.Direction;

			float arrowScale = glm::clamp(glm::length(fromCameraDirection), 1.0f, 1.5f);

			up *= arrowScale;

			auto* rayVertices = &m_Vertices[rayIndex * DebugRendererSettings::VerticesPerRay];

			rayVertices[0].Position = ray.Origin - up;
			rayVertices[1].Position = ray.Origin + up;
			rayVertices[2].Position = end + up;
			rayVertices[3].Position = end - up;

			rayVertices[4].Position = end + up * 2.0f;
			rayVertices[5].Position = end + glm::normalize(ray.Direction) * triangleHeight * arrowScale;
			rayVertices[6].Position = end - up * 2.0f;
		}
	}
}
