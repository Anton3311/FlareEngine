#include "DebugRenderer.h"

#include "FlareCore/Assert.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer/Shader.h"
#include "Flare/Renderer/SceneSubmition.h"
#include "Flare/Renderer/Viewport.h"

#include "Flare/Project/Project.h"

#include "Flare/DebugRenderer/DebugRendererFrameData.h"
#include "Flare/DebugRenderer/DebugLinesPass.h"
#include "Flare/DebugRenderer/DebugRaysPass.h"

namespace Flare
{
	using Vertex = DebugRendererFrameData::Vertex;

	struct DebugRendererData
	{
		SceneSubmition* SceneSubmition = nullptr;
		DebugRendererFrameData* Submition = nullptr;

		Ref<Shader> DebugShader = nullptr;
		Ref<IndexBuffer> RaysIndexBuffer = nullptr;

		DebugRendererSettings Settings;
	};

	DebugRendererData s_DebugRendererData;

	static void ReloadShader()
	{
		FLARE_PROFILE_FUNCTION();
		s_DebugRendererData.DebugShader = nullptr;

		AssetHandle debugShaderHandle = ShaderLibrary::FindShader("Debug").value_or(NULL_ASSET_HANDLE);
		if (AssetManager::IsAssetHandleValid(debugShaderHandle))
		{
			s_DebugRendererData.DebugShader = AssetManager::GetAsset<Shader>(debugShaderHandle);
		}
	}

	void DebugRenderer::Initialize()
	{
		FLARE_PROFILE_FUNCTION();
		s_DebugRendererData.Settings.MaxRays = 10000;
		s_DebugRendererData.Settings.MaxLines = 40000;
		s_DebugRendererData.Settings.RayThickness = 0.07f;

		size_t indexBufferSize = s_DebugRendererData.Settings.MaxRays * DebugRendererSettings::IndicesPerRay;
		uint32_t* indices = new uint32_t[indexBufferSize];
		uint32_t vertexIndex = 0;

		for (uint32_t index = 0; index < indexBufferSize; index += DebugRendererSettings::IndicesPerRay)
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

			vertexIndex += DebugRendererSettings::VerticesPerRay;
		}

		s_DebugRendererData.RaysIndexBuffer = IndexBuffer::Create(IndexBuffer::IndexFormat::UInt32, MemorySpan(indices, indexBufferSize));

		delete[] indices;

		Project::OnProjectOpen.Bind(ReloadShader);
	}

	void DebugRenderer::Shutdown()
	{
		s_DebugRendererData = {};
	}

	void DebugRenderer::BeginScene(SceneSubmition& sceneSubmition)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(s_DebugRendererData.SceneSubmition == nullptr);

		s_DebugRendererData.SceneSubmition = &sceneSubmition;
		s_DebugRendererData.Submition = &sceneSubmition.DebugRendererSubmition;

		s_DebugRendererData.Submition->LineVertices.resize(s_DebugRendererData.Settings.MaxLines * 2);
		s_DebugRendererData.Submition->Rays.resize(s_DebugRendererData.Settings.MaxRays * 2);
	}

	void DebugRenderer::EndScene()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(s_DebugRendererData.SceneSubmition != nullptr);

		s_DebugRendererData.SceneSubmition = nullptr;
		s_DebugRendererData.Submition = nullptr;
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
		FLARE_CORE_ASSERT(s_DebugRendererData.Submition->LineCount < s_DebugRendererData.Settings.MaxLines);
		uint32_t lineIndex = s_DebugRendererData.Submition->LineCount;

		Vertex& startVertex = s_DebugRendererData.Submition->LineVertices[lineIndex * 2 + 0];
		Vertex& endVertex = s_DebugRendererData.Submition->LineVertices[lineIndex * 2 + 1];

		startVertex.Position = start;
		startVertex.Color = startColor;

		endVertex.Position = end;
		endVertex.Color = endColor;

		s_DebugRendererData.Submition->LineCount++;
	}

	void DebugRenderer::DrawRay(const glm::vec3& origin, const glm::vec3& direction, const glm::vec4& color)
	{
		FLARE_CORE_ASSERT(s_DebugRendererData.Submition->RayCount < s_DebugRendererData.Settings.MaxRays);
		DebugRayData& ray = s_DebugRendererData.Submition->Rays[s_DebugRendererData.Submition->RayCount];
		ray.Origin = origin;
		ray.Direction = direction;
		ray.Color = color;

		s_DebugRendererData.Submition->RayCount++;
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
		FLARE_PROFILE_FUNCTION();
		if (!viewport.IsDebugRenderingEnabled())
			return;

		RenderGraphPassSpecifications linesPass{};
		linesPass.AddOutput(viewport.ColorTextureId, 0);
		linesPass.AddOutput(viewport.DepthTextureId, 1);
		linesPass.SetType(RenderGraphPassType::Graphics);
		linesPass.SetDebugName("DebugLinesPass");

		viewport.Graph.AddPass(linesPass, CreateRef<DebugLinesPass>(
			s_DebugRendererData.DebugShader,
			s_DebugRendererData.Settings));

		RenderGraphPassSpecifications raysPass{};
		raysPass.AddOutput(viewport.ColorTextureId, 0);
		raysPass.AddOutput(viewport.DepthTextureId, 1);
		raysPass.SetType(RenderGraphPassType::Graphics);
		raysPass.SetDebugName("DebugRaysPass");

		viewport.Graph.AddPass(raysPass, CreateRef<DebugRaysPass>(
			s_DebugRendererData.RaysIndexBuffer,
			s_DebugRendererData.DebugShader,
			s_DebugRendererData.Settings));
	}
}
