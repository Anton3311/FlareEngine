#pragma once

#include "Flare/Renderer/CommandBuffer.h"

#include "Flare/Platform/OpenGL/OpenGLFrameBuffer.h"
#include "Flare/Renderer/ShaderMetadata.h"

namespace Flare
{
	class OpenGLCommandBuffer : public CommandBuffer
	{
	public:
		~OpenGLCommandBuffer();

		void BeginRenderTarget(const Ref<FrameBuffer> frameBuffer) override;
		void EndRenderTarget() override;

		void ClearColorAttachment(Ref<FrameBuffer> frameBuffer, uint32_t index, const glm::vec4& clearColor) override;
		void ClearDepthAttachment(Ref<FrameBuffer> frameBuffer, float depth) override;

		void ApplyMaterial(const Ref<const Material>& material) override;

		void SetViewportAndScisors(Math::Rect viewportRect) override;

		void DrawIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t baseInstance, uint32_t instanceCount) override;

		void Blit(Ref<FrameBuffer> source, uint32_t sourceAttachment, Ref<FrameBuffer> destination, uint32_t destinationAttachment, TextureFiltering filter) override;

		void StartTimer(Ref<GPUTimer> timer) override;
		void StopTimer(Ref<GPUTimer> timer) override;

		void SetAttachmentsWriteMask(uint32_t mask);
	private:
		void SetDepthTestEnabled(bool enabled);
		void SetCullingMode(CullingMode mode);
		void SetDepthComparisonFunction(DepthComparisonFunction function);
		void SetDepthWriteEnabled(bool enabled);
		void SetBlendMode(BlendMode mode);
	private:
		Ref<OpenGLFrameBuffer> m_CurrentRenderTarget = nullptr;
	};
}
