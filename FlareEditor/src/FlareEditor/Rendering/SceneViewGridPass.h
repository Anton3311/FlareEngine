#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/ShaderConstantBuffer.h"

namespace Flare
{
	struct SceneViewGridSettings
	{
		float MaxVisibleDistance = 80.0f;
		glm::vec3 PrimaryColor = glm::vec3(0.9f);
		glm::vec3 SecondaryColor = glm::vec3(0.6f);
	};

	class VertexBuffer;
	class Shader;
	class Pipeline;
	class SceneViewGridPass : public RenderGraphPass
	{
	public:
		SceneViewGridPass();

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		void GenerateGridMesh();
		void CreatePipeline(const RenderGraphContext& context);
		void DrawGridLevel(Ref<CommandBuffer> commandBuffer, int32_t level, glm::vec3 color);
	private:
		Ref<Shader> m_Shader = nullptr;
		Ref<Pipeline> m_Pipeline = nullptr;
		Ref<VertexBuffer> m_VertexBuffer = nullptr;

		SceneViewGridSettings m_Settings;

		ShaderConstantBuffer m_ConstantBuffer;

		uint32_t m_CellCount = 0;
		uint32_t m_VertexCount = 0;

		uint32_t m_CellSizePropertyIndex = UINT32_MAX;
		uint32_t m_ScalePropertyIndex = UINT32_MAX;
		uint32_t m_FadeDistancePropertyIndex = UINT32_MAX;
		uint32_t m_ColorPropertyIndex = UINT32_MAX;
	};
}
