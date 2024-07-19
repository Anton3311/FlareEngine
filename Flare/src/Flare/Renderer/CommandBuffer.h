#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include "Flare/Math/Math.h"

#include "Flare/Renderer/Texture.h"

namespace Flare
{
	class FrameBuffer;
	class Material;
	class Mesh;
	class GPUTimer;
	class ComputePipeline;
	class Pipeline;
	class DescriptorSet;
	class VertexBuffer;
	class IndexBuffer;
	class ShaderConstantBuffer;

	class CommandBuffer
	{
	public:
		virtual void BeginRenderTarget(const Ref<FrameBuffer> frameBuffer) = 0;
		virtual void EndRenderTarget() = 0;

		virtual void ClearColorAttachment(Ref<FrameBuffer> frameBuffer, uint32_t index, const glm::vec4& clearColor) = 0;

		// Depth is in range [0.0, 1.0]
		// 0.0 - is near plane
		// 1.0 - is far plane
		virtual void ClearDepthAttachment(Ref<FrameBuffer> frameBuffer, float depth) = 0;

		// Depth is in range [0.0, 1.0]
		// 0.0 - is near plane
		// 1.0 - is far plane
		virtual void ClearColor(const Ref<Texture>& texture, const glm::vec4& clearColor) = 0;
		virtual void ClearDepth(const Ref<Texture>& texture, float depth) = 0;

		virtual void ApplyMaterial(const Ref<const Material>& material) = 0;
		virtual void PushConstants(const ShaderConstantBuffer& constantBuffer) = 0;

		virtual void SetViewportAndScisors(Math::Rect viewportRect) = 0;

		virtual void BindPipeline(Ref<Pipeline> pipeline) = 0;
		virtual void BindVertexBuffer(Ref<const VertexBuffer> buffer, uint32_t index) = 0;
		virtual void BindVertexBuffers(Span<Ref<const VertexBuffer>> buffers, uint32_t baseBindingIndex) = 0;
		virtual void BindIndexBuffer(Ref<const IndexBuffer> buffer) = 0;

		virtual void DrawMeshIndexed(const Ref<const Mesh>& mesh, uint32_t baseInstance, uint32_t instanceCount) = 0;
		virtual void DrawMeshIndexed(const Ref<const Mesh>& mesh,
			uint32_t subMeshIndex,
			uint32_t baseInstance,
			uint32_t instanceCount) = 0;

		virtual void DrawIndexed(uint32_t baseIndex,
			uint32_t indexCount,
			uint32_t vertexOffset,
			uint32_t baseInstance,
			uint32_t instanceCount) = 0;

		virtual void Draw(uint32_t baseVertex,
			uint32_t vertexCount,
			uint32_t baseInstance,
			uint32_t instanceCount) = 0;

		virtual void Blit(Ref<const Texture> source, Ref<const Texture> destination, TextureFiltering filter) = 0;

		virtual void SetGlobalDescriptorSet(Ref<const DescriptorSet> set, uint32_t index) = 0;

		virtual void DispatchCompute(Ref<ComputePipeline> pipeline, const glm::uvec3& groupCount) = 0;

		virtual void StartTimer(Ref<GPUTimer> timer) = 0;
		virtual void StopTimer(Ref<GPUTimer> timer) = 0;
	};
}
