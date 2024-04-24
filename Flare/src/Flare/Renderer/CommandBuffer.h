#pragma once

#include "FlareCore/Core.h"
#include "Flare/Math/Math.h"

#include "Flare/Renderer/Texture.h"

namespace Flare
{
	class FrameBuffer;
	class Material;
	class Mesh;
	class GPUTimer;

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

		virtual void ApplyMaterial(const Ref<const Material>& material) = 0;

		virtual void SetViewportAndScisors(Math::Rect viewportRect) = 0;

		virtual void DrawIndexed(const Ref<const Mesh>& mesh,
			uint32_t subMeshIndex,
			uint32_t baseInstance,
			uint32_t instanceCount) = 0;

		virtual void Blit(Ref<FrameBuffer> source, uint32_t sourceAttachment, Ref<FrameBuffer> destination, uint32_t destinationAttachment, TextureFiltering filter) = 0;

		virtual void StartTimer(Ref<GPUTimer> timer) = 0;
		virtual void StopTimer(Ref<GPUTimer> timer) = 0;
	};
}
