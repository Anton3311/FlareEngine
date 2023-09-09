#include "DebugRenderer.h"

#include "FlareCore/Assert.h"

#include "Flare/Renderer/VertexArray.h"
#include "Flare/Renderer/Buffer.h"
#include "Flare/Renderer/RenderCommand.h"
#include "Flare/Renderer/Shader.h"

namespace Flare
{
	struct LineVertex
	{
		glm::vec3 Position;
		glm::vec4 Color;
	};

	struct DebugRendererData
	{
		const RenderData* FrameData;

		LineVertex* LinesBufferBase;
		LineVertex* LinesBuffer;

		uint32_t LinesCount;
		uint32_t MaxLinesCount;

		Ref<Shader> LineShader;
		Ref<VertexBuffer> LinesVertexBuffer;
		Ref<VertexArray> LinesMesh;
	};

	DebugRendererData s_DebugRendererData;

	void DebugRenderer::Initialize(uint32_t maxLinesCount)
	{
		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(sizeof(LineVertex) * 2 * maxLinesCount);
		vertexBuffer->SetLayout({
			{ "i_Position", ShaderDataType::Float3, false },
			{ "i_Color", ShaderDataType::Float4, false },
		});

		Ref<VertexArray> mesh = VertexArray::Create();
		mesh->AddVertexBuffer(vertexBuffer);

		s_DebugRendererData.FrameData = nullptr;

		s_DebugRendererData.LineShader = Shader::Create("assets/Shaders/DebugLine.glsl");

		s_DebugRendererData.LinesVertexBuffer = vertexBuffer;
		s_DebugRendererData.LinesMesh = mesh;

		s_DebugRendererData.MaxLinesCount = maxLinesCount;
		s_DebugRendererData.LinesBufferBase = new LineVertex[s_DebugRendererData.MaxLinesCount * 2];
		s_DebugRendererData.LinesBuffer = s_DebugRendererData.LinesBufferBase;

		mesh->Unbind();
	}

	void DebugRenderer::Shutdown()
	{
		FLARE_CORE_ASSERT(s_DebugRendererData.LinesBufferBase);

		delete[] s_DebugRendererData.LinesBufferBase;

		s_DebugRendererData.LinesBufferBase = nullptr;
		s_DebugRendererData.LinesBuffer = nullptr;
	}

	void DebugRenderer::Begin(const RenderData& renderData)
	{
		FLARE_CORE_ASSERT(!s_DebugRendererData.FrameData);
		s_DebugRendererData.FrameData = &renderData;
	}

	void DebugRenderer::End()
	{
		FlushLines();
		s_DebugRendererData.FrameData = nullptr;
	}

	void DebugRenderer::FlushLines()
	{
		FLARE_CORE_ASSERT(s_DebugRendererData.FrameData);
		s_DebugRendererData.LinesVertexBuffer->SetData(s_DebugRendererData.LinesBufferBase, sizeof(LineVertex) * 2 * s_DebugRendererData.LinesCount);

		s_DebugRendererData.LineShader->Bind();
		s_DebugRendererData.LineShader->SetMatrix4("u_ViewProjection", s_DebugRendererData.FrameData->Camera.ViewProjectionMatrix);

		RenderCommand::DrawLines(s_DebugRendererData.LinesMesh, s_DebugRendererData.LinesCount * 2);

		s_DebugRendererData.LinesCount = 0;
		s_DebugRendererData.LinesBuffer = s_DebugRendererData.LinesBufferBase;
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

		s_DebugRendererData.LinesCount++;

		if (s_DebugRendererData.LinesCount == s_DebugRendererData.MaxLinesCount)
			FlushLines();
	}
}
