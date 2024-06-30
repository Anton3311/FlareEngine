#include "SceneViewGridPass.h"

#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Material.h"

#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"

#include "Flare/AssetManager/AssetManager.h"

namespace Flare
{
	SceneViewGridPass::SceneViewGridPass()
	{
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("SceneViewGrid");
		FLARE_CORE_ASSERT(shaderHandle.has_value());
		FLARE_CORE_ASSERT(AssetManager::IsAssetHandleValid(*shaderHandle));

		m_Shader = AssetManager::GetAsset<Shader>(*shaderHandle);

		m_CellCount = 20;

		GenerateGridMesh();
	}

	void SceneViewGridPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		if (m_Pipeline == nullptr)
			CreatePipeline(context);

		commandBuffer->BeginRenderTarget(context.GetRenderTarget());

		Ref<VulkanCommandBuffer> vulkanCommandBuffer = As<VulkanCommandBuffer>(commandBuffer);
		Ref<VulkanPipeline> pipeline = As<VulkanPipeline>(m_Pipeline);

		vulkanCommandBuffer->BindPipeline(m_Pipeline);
		vulkanCommandBuffer->BindDescriptorSet(As<VulkanDescriptorSet>(Renderer::GetPrimaryDescriptorSet()), pipeline->GetLayoutHandle(), 0);
		vulkanCommandBuffer->BindVertexBuffers(Span((Ref<const VertexBuffer>*) & m_VertexBuffer, 1));
		vulkanCommandBuffer->Draw(0, m_VertexCount, 0, 1);

		commandBuffer->EndRenderTarget();
	}

	void SceneViewGridPass::GenerateGridMesh()
	{
		uint32_t lineCount = (m_CellCount + 1) * 2; // Vertical + horizontal
		std::vector<glm::vec2> vertices;
		vertices.reserve(lineCount * 2);

		float offset = 0.5f;
		float spacing = 1.0f / (float)m_CellCount;

		// Generate vertical lines
		for (uint32_t lineIndex = 0; lineIndex <= m_CellCount; lineIndex++)
		{
			glm::vec2 start = glm::vec2(spacing * lineIndex - offset, -0.5f);
			glm::vec2 end = glm::vec2(start.x, 0.5f);

			vertices.push_back(start);
			vertices.push_back(end);
		}

		// Generate horizontal lines by copying and rotating vertical ones
		for (uint32_t lineIndex = 0; lineIndex <= m_CellCount; lineIndex++)
		{
			uint32_t vertexIndex = lineIndex * 2;

			glm::vec2 start = vertices[vertexIndex + 0];
			glm::vec2 end = vertices[vertexIndex + 1];

			std::swap(start.x, start.y);
			std::swap(end.x, end.y);

			vertices.push_back(start);
			vertices.push_back(end);
		}

		m_VertexBuffer = VertexBuffer::Create(sizeof(glm::vec2) * vertices.size(), vertices.data());
		m_VertexCount = (uint32_t)vertices.size();
	}

	void SceneViewGridPass::CreatePipeline(const RenderGraphContext& context)
	{
		PipelineSpecifications specifications{};
		specifications.Shader = m_Shader;
		specifications.DepthTest = true;
		specifications.DepthWrite = false;
		specifications.Blending = BlendMode::Transparent;
		specifications.Culling = CullingMode::None;
		specifications.Topology = PrimitiveTopology::Lines;
		specifications.DepthBiasConstantFactor = 0.0f;
		specifications.DepthBiasSlopeFactor = 0.0f;
		specifications.DepthBiasEnabled = false;
		specifications.DepthFunction = DepthComparisonFunction::Less;
		specifications.InputLayout = PipelineInputLayout({
			PipelineInputLayoutElement(0, 0, ShaderDataType::Float2)
		});

		Ref<VulkanFrameBuffer> renderTarget = As<VulkanFrameBuffer>(context.GetRenderTarget());
		m_Pipeline = CreateRef<VulkanPipeline>(specifications, renderTarget->GetCompatibleRenderPass());
	}
}
