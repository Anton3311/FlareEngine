#pragma once

#include "Flare/Renderer/CommandBuffer.h"

#include "Flare/Platform/OpenGL/OpenGLFrameBuffer.h"

namespace Flare
{
	class OpenGLCommandBuffer : public CommandBuffer
	{
	public:
		~OpenGLCommandBuffer();

		void BeginRenderTarget(const Ref<FrameBuffer> frameBuffer) override;
		void EndRenderTarget() override;

		void ApplyMaterial(const Ref<const Material>& material) override;

		void SetViewportAndScisors(Math::Rect viewportRect) override;

		void DrawIndexed(const Ref<const Mesh>& mesh, uint32_t subMeshIndex, uint32_t baseInstance, uint32_t instanceCount) override;

		void StartTimer(Ref<GPUTimer> timer) override;
		void StopTimer(Ref<GPUTimer> timer) override;
	private:
		Ref<OpenGLFrameBuffer> m_CurrentRenderTarget = nullptr;
	};
}
