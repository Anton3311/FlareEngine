#pragma once

#include "Flare/Renderer/RendererAPI.h"

namespace Flare
{
	class VulkanRendererAPI : public RendererAPI
	{
	public:
		void Initialize() override;
		void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		void SetClearColor(float r, float g, float b, float a) override;
		void Clear() override;
		void SetDepthTestEnabled(bool enabled) override;
		void SetCullingMode(CullingMode mode) override;
		void SetDepthComparisonFunction(DepthComparisonFunction function) override;
		void SetDepthWriteEnabled(bool enabled) override;
		void SetBlendMode(BlendMode mode) override;
		void SetLineWidth(float width) override;
		void DrawIndexed(const Ref<const VertexArray>& vertexArray) override;
		void DrawIndexed(const Ref<const VertexArray>& vertexArray, size_t indicesCount) override;
		void DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount) override;
		void DrawInstancesIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t instancesCount, uint32_t baseInstance) override;
		void DrawInstancesIndexedIndirect(const Ref<const Mesh>& mesh, const Span<DrawIndirectCommandSubMeshData>& subMeshesData, uint32_t baseInstance) override;
		void DrawLines(const Ref<const VertexArray>& vertexArray, size_t cverticesCountount) override;
		void DrawInstanced(const Ref<const VertexArray>& mesh, size_t instancesCount, size_t baseVertexIndex, size_t startIndex, size_t indicesCount) override;
		void ApplyMaterialProperties(const Ref<const Material>& materail) override;
	};
}
