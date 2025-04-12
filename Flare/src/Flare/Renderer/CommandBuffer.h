#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Collections/Span.h"

#include "Flare/Renderer/Buffer.h"

#include "Flare/Math/Math.h"

#include "Flare/Renderer/Texture.h"

namespace Flare
{
	class Material;
	class Mesh;
	class GPUBuffer;
	class GPUTimer;
	class ComputeShader;
	class Pipeline;
	class DescriptorSet;
	class ShaderConstantBuffer;
	class ShaderDescriptorBuffer;

	class FLARE_API CommandBuffer : public RefCounted<CommandBuffer>
	{
	public:
		virtual void BeginLabel(const glm::vec4& color, const std::string& label) = 0;
		virtual void EndLabel() = 0;

		// Depth is in range [0.0, 1.0]
		// 0.0 - is near plane
		// 1.0 - is far plane
		virtual void ClearColor(const Ref<Texture>& texture, const glm::vec4& clearColor) = 0;
		virtual void ClearDepth(const Ref<Texture>& texture, float depth) = 0;

		virtual void ApplyMaterial(const Ref<const Material>& material) = 0;

		virtual void PushDescriptorProperties(ShaderDescriptorBuffer& descriptorProperties) = 0;
		virtual void PushConstants(const ShaderConstantBuffer& constantBuffer) = 0;

		virtual void SetViewportAndScissors(Math::Rect viewportRect) = 0;
		virtual void SetDefaultViewportAndScissors() = 0;

		virtual void BindPipeline(const Ref<Pipeline>& pipeline) = 0;
		virtual void BindVertexBuffer(Ref<const GPUBuffer> buffer, uint32_t index) = 0;
		virtual void BindVertexBuffers(Span<Ref<const GPUBuffer>> buffers, uint32_t baseBindingIndex) = 0;
		virtual void BindIndexBuffer(Ref<const GPUBuffer> buffe, IndexFormat formatr) = 0;

		virtual void DrawMeshIndexed(const Ref<const Mesh>& mesh, uint32_t baseInstance, uint32_t instanceCount) = 0;

		virtual void DrawDepthOnlyMeshIndexed(const Ref<const Mesh>& mesh,
				uint32_t baseInstance,
				uint32_t instanceCount) = 0;

		virtual void DrawDepthOnlyMeshIndexed(const Ref<const Mesh>& mesh,
				uint32_t subMeshIndex,
				uint32_t baseInstance,
				uint32_t instanceCount) = 0;

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

		virtual void BindComputeShader(Ref<ComputeShader> computeShader) = 0;
		virtual void DispatchCompute(const glm::uvec3& groupCount) = 0;

		virtual void StartTimer(Ref<GPUTimer> timer) = 0;
		virtual void StopTimer(Ref<GPUTimer> timer) = 0;
	};
}
