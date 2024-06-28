#include "DebugRenderer.h"

#include "FlareCore/Assert.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"

#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Shader.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanPipeline.h"
#include "Flare/Platform/Vulkan/VulkanRenderPass.h"

#include "Flare/Renderer/Pipeline.h"

#include "Flare/Project/Project.h"

#include "Flare/DebugRenderer/DebugRendererFrameData.h"
#include "Flare/DebugRenderer/DebugLinesPass.h"

namespace Flare
{
	using Vertex = DebugRenderFrameData::Vertex;

	struct DebugRendererData
	{
		Vertex* LinesBufferBase = nullptr;
		Vertex* LinesBuffer = nullptr;

		Vertex* RaysBufferBase = nullptr;
		Vertex* RaysBuffer = nullptr;

		uint32_t LineCount = 0;
		uint32_t RayCount = 0;

		Ref<Shader> DebugShader = nullptr;

		Ref<VertexBuffer> RaysVertexBuffer = nullptr;
		Ref<IndexBuffer> RaysIndexBuffer = nullptr;

		Ref<Pipeline> RayPipeline = nullptr;

		DebugRendererSettings Settings;
		DebugRenderFrameData FrameData;
	};

	DebugRendererData s_DebugRendererData;

	constexpr uint32_t VerticesPerLine = 2;
	constexpr uint32_t VerticesPerRay = 7;
	constexpr uint32_t IndicesPerRay = 9;

	static void ReloadShader()
	{
		s_DebugRendererData.DebugShader = nullptr;

		AssetHandle debugShaderHandle = ShaderLibrary::FindShader("Debug").value_or(NULL_ASSET_HANDLE);
		if (AssetManager::IsAssetHandleValid(debugShaderHandle))
		{
			s_DebugRendererData.DebugShader = AssetManager::GetAsset<Shader>(debugShaderHandle);
		}
	}

	void DebugRenderer::Initialize()
	{
		s_DebugRendererData.Settings.MaxRays = 10000;
		s_DebugRendererData.Settings.MaxLines = 40000;
		s_DebugRendererData.Settings.RayThickness = 0.07f;

		uint32_t* indices = new uint32_t[s_DebugRendererData.Settings.MaxRays * IndicesPerRay];
		uint32_t vertexIndex = 0;

		for (uint32_t index = 0; index < s_DebugRendererData.Settings.MaxRays * IndicesPerRay; index += IndicesPerRay)
		{
			indices[index + 0] = vertexIndex + 0;
			indices[index + 1] = vertexIndex + 1;
			indices[index + 2] = vertexIndex + 2;
			indices[index + 3] = vertexIndex + 0;
			indices[index + 4] = vertexIndex + 2;
			indices[index + 5] = vertexIndex + 3;

			indices[index + 6] = vertexIndex + 4;
			indices[index + 7] = vertexIndex + 5;
			indices[index + 8] = vertexIndex + 6;

			vertexIndex += VerticesPerRay;
		}

		delete[] indices;

		s_DebugRendererData.RayCount = 0;
		s_DebugRendererData.LineCount = 0;

		s_DebugRendererData.LinesBufferBase = new Vertex[s_DebugRendererData.Settings.MaxLines * VerticesPerLine];
		s_DebugRendererData.LinesBuffer = s_DebugRendererData.LinesBufferBase;

		s_DebugRendererData.RaysBufferBase = new Vertex[s_DebugRendererData.Settings.MaxRays * VerticesPerRay];
		s_DebugRendererData.RaysBuffer = s_DebugRendererData.RaysBufferBase;

		Project::OnProjectOpen.Bind(ReloadShader);
	}

	void DebugRenderer::Shutdown()
	{
		FLARE_CORE_ASSERT(s_DebugRendererData.LinesBufferBase);
		FLARE_CORE_ASSERT(s_DebugRendererData.RaysBufferBase);

		delete[] s_DebugRendererData.LinesBufferBase;
		delete[] s_DebugRendererData.RaysBufferBase;

		s_DebugRendererData.LinesBufferBase = nullptr;
		s_DebugRendererData.LinesBuffer = nullptr;

		s_DebugRendererData = {};
	}

	void DebugRenderer::Begin()
	{
		s_DebugRendererData.LinesBuffer = s_DebugRendererData.LinesBufferBase;
		s_DebugRendererData.LineCount = 0;
	}

	void DebugRenderer::End()
	{
		s_DebugRendererData.FrameData.LineVertices = Span(s_DebugRendererData.LinesBufferBase, s_DebugRendererData.LineCount);
	}

	void DebugRenderer::DrawLine(const glm::vec3& start, const glm::vec3& end)
	{
		DrawLine(start, end, glm::vec4(1.0f), glm::vec4(1.0f));
	}

	void DebugRenderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color)
	{
		DrawLine(start, end, color, color);
	}

	void DebugRenderer::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& startColor, const glm::vec4& endColor)
	{
		s_DebugRendererData.LinesBuffer->Position = start;
		s_DebugRendererData.LinesBuffer->Color = startColor;
		s_DebugRendererData.LinesBuffer++;

		s_DebugRendererData.LinesBuffer->Position = end;
		s_DebugRendererData.LinesBuffer->Color = endColor;
		s_DebugRendererData.LinesBuffer++;

		s_DebugRendererData.LineCount++;
	}

	void DebugRenderer::DrawRay(const glm::vec3& origin, const glm::vec3& direction, const glm::vec4& color)
	{
#if 0
		FLARE_CORE_ASSERT(s_DebugRendererData.CurrentViewport != nullptr);

		glm::vec3 fromCameraDirection = s_DebugRendererData.CurrentViewport->FrameData.Camera.Position - origin;
		glm::vec3 up = glm::normalize(glm::cross(fromCameraDirection, direction)) * s_DebugRendererData.Settings.RayThickness / 2.0f;

		for (uint32_t i = 0; i < VerticesPerRay; i++)
			s_DebugRendererData.RaysBuffer[i].Color = color;

		glm::vec3 end = origin + direction;

		float triangleHeight = glm::sqrt(3.0f) * s_DebugRendererData.RayThickness;
		float arrowScale = glm::clamp(glm::length(fromCameraDirection), 1.0f, 1.5f);

		up *= arrowScale;
		triangleHeight *= arrowScale;

		s_DebugRendererData.RaysBuffer[0].Position = origin - up;
		s_DebugRendererData.RaysBuffer[1].Position = origin + up;
		s_DebugRendererData.RaysBuffer[2].Position = end + up;
		s_DebugRendererData.RaysBuffer[3].Position = end - up;

		s_DebugRendererData.RaysBuffer[4].Position = end + up * 2.0f;
		s_DebugRendererData.RaysBuffer[5].Position = end + glm::normalize(direction) * triangleHeight;
		s_DebugRendererData.RaysBuffer[6].Position = end - up * 2.0f;

		s_DebugRendererData.RayCount++;
		s_DebugRendererData.RaysBuffer += VerticesPerRay;
#endif
	}

	void DebugRenderer::DrawCircle(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, float radius, const glm::vec4& color, uint32_t segments)
	{
		glm::vec3 bitangent = glm::cross(tangent, normal);

		float segmentAngle = glm::pi<float>() * 2.0f / (float)segments;
		float currentAngle = 0.0f;

		glm::vec3 previousPoint(0.0f);
		for (uint32_t i = 0; i <= segments; i++)
		{
			glm::vec2 point = glm::vec2(glm::cos(currentAngle), glm::sin(currentAngle)) * radius;
			glm::vec3 worldSpacePoint = position + tangent * point.x + bitangent * point.y;

			if (i > 0)
			{
				DrawLine(previousPoint, worldSpacePoint, color);
			}

			previousPoint = worldSpacePoint;
			currentAngle += segmentAngle;
		}
	}

	void DebugRenderer::DrawWireSphere(const glm::vec3& origin, float radius, const glm::vec4& color, uint32_t segments)
	{
		DrawCircle(origin, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, color, segments);
		DrawCircle(origin, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), radius, color, segments);
		DrawCircle(origin, glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), radius, color, segments);
	}

	void DebugRenderer::DrawWireQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
	{
		glm::vec3 halfSize = glm::vec3(size / 2.0f, 0.0f);
		DrawLine(position + glm::vec3(-halfSize.x, halfSize.y, 0.0f), position + halfSize, color);
		DrawLine(position + glm::vec3(-halfSize.x, -halfSize.y, 0.0f), position + glm::vec3(halfSize.x, -halfSize.y, 0.0f), color);
		DrawLine(position + glm::vec3(-halfSize.x, halfSize.y, 0.0f), position + glm::vec3(-halfSize.x, -halfSize.y, 0.0f), color);
		DrawLine(position + halfSize, position + glm::vec3(halfSize.x, -halfSize.y, 0.0f), color);
	}

	void DebugRenderer::DrawWireQuad(const glm::vec3* corners, const glm::vec4& color)
	{
		DrawLine(corners[0], corners[1], color);
		DrawLine(corners[1], corners[2], color);
		DrawLine(corners[2], corners[3], color);
		DrawLine(corners[3], corners[0], color);
	}

	void DebugRenderer::DrawFrustum(const glm::mat4& inverseViewProjection, const glm::vec4& color)
	{
		std::array<glm::vec4, 8> frustumCorners =
		{
			glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f),
			glm::vec4(1.0f,  1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(-1.0f,  1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f,  1.0f, 1.0f, 1.0f),
		};

		for (size_t i = 0; i < frustumCorners.size(); i++)
		{
			frustumCorners[i] = inverseViewProjection * frustumCorners[i];
			frustumCorners[i] /= frustumCorners[i].w;
		}

		for (size_t i = 0; i < 8; i += 4)
		{
			DrawLine(frustumCorners[i + 0], frustumCorners[i + 1], color);
			DrawLine(frustumCorners[i + 0], frustumCorners[i + 2], color);
			DrawLine(frustumCorners[i + 2], frustumCorners[i + 3], color);
			DrawLine(frustumCorners[i + 1], frustumCorners[i + 3], color);
		}

		for (size_t i = 0; i < 4; i++)
			DrawLine(frustumCorners[i], frustumCorners[i + 4], color);
	}

	void DebugRenderer::DrawWireBox(const glm::vec3 corners[8], const glm::vec4& color)
	{
		for (size_t i = 0; i < 8; i += 4)
		{
			DrawLine(corners[i], corners[i + 1], color);
			DrawLine(corners[i], corners[i + 2], color);
			DrawLine(corners[i + 1], corners[i + 3], color);
			DrawLine(corners[i + 2], corners[i + 3], color);
		}

		for (size_t i = 0; i < 4; i++)
		{
			DrawLine(corners[i], corners[i + 4], color);
		}
	}

	void DebugRenderer::DrawAABB(const Math::AABB& aabb, const glm::vec4& color)
	{
		glm::vec3 corners[8];
		aabb.GetCorners(corners);

		DrawWireBox(corners, color);
	}

	void DebugRenderer::ConfigurePasses(Viewport& viewport)
	{
		RenderGraphPassSpecifications linesPass{};
		linesPass.AddOutput(viewport.ColorTexture, 0);
		linesPass.AddOutput(viewport.DepthTexture, 1);
		linesPass.SetType(RenderGraphPassType::Graphics);
		linesPass.SetDebugName("DebugLinesPass");

		viewport.Graph.AddPass(linesPass, CreateRef<DebugLinesPass>(
			s_DebugRendererData.DebugShader,
			s_DebugRendererData.Settings,
			s_DebugRendererData.FrameData));
	}

	void DebugRenderer::FlushRays()
	{
#if 0
		if (s_DebugRendererData.DebugShader)
		{
			s_DebugRendererData.RaysVertexBuffer->SetData(s_DebugRendererData.RaysBufferBase, sizeof(Vertex) * VerticesPerRay * s_DebugRendererData.RaysCount);

			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
			{
				if (s_DebugRendererData.RayPipeline == nullptr)
				{
					PipelineSpecifications rayPipelineSpecifications{};
					rayPipelineSpecifications.Blending = BlendMode::Opaque;
					rayPipelineSpecifications.Culling = CullingMode::None;
					rayPipelineSpecifications.DepthFunction = DepthComparisonFunction::Less;
					rayPipelineSpecifications.DepthTest = true;
					rayPipelineSpecifications.DepthWrite = false;
					rayPipelineSpecifications.Shader = s_DebugRendererData.DebugShader;
					rayPipelineSpecifications.Topology = PrimitiveTopology::Triangles;
					rayPipelineSpecifications.InputLayout = PipelineInputLayout({
						{ 0, 0, ShaderDataType::Float3 },
						{ 0, 1, ShaderDataType::Float4 }
					});

					s_DebugRendererData.RayPipeline = CreateRef<VulkanPipeline>(
						rayPipelineSpecifications,
						As<VulkanFrameBuffer>(s_DebugRendererData.CurrentViewport->RenderTarget)->GetCompatibleRenderPass());
				}

				Ref<VulkanCommandBuffer> commandBuffer = As<VulkanCommandBuffer>(GraphicsContext::GetInstance().GetCommandBuffer());

				commandBuffer->BeginRenderTarget(s_DebugRendererData.CurrentViewport->RenderTarget);

				commandBuffer->BindVertexBuffers(Span((Ref<const VertexBuffer>*)&s_DebugRendererData.RaysVertexBuffer, 1));
				commandBuffer->BindIndexBuffer(s_DebugRendererData.RaysIndexBuffer);

				commandBuffer->BindPipeline(s_DebugRendererData.RayPipeline);
				commandBuffer->BindDescriptorSet(
					As<VulkanDescriptorSet>(Renderer::GetPrimaryDescriptorSet()),
					As<VulkanPipeline>(s_DebugRendererData.RayPipeline)->GetLayoutHandle(),
					0);

				commandBuffer->DrawIndexed(0, s_DebugRendererData.RaysCount * IndicesPerRay, 0, 1);

				commandBuffer->EndRenderTarget();
			}
		}

		s_DebugRendererData.RaysCount = 0;
		s_DebugRendererData.RaysBuffer = s_DebugRendererData.RaysBufferBase;
#endif
	}
}
