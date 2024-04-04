#pragma once

#include "FlareCore/Core.h"
#include "Flare/Math/Math.h"

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

		virtual void ApplyMaterial(const Ref<const Material>& material) = 0;

		virtual void SetViewportAndScisors(Math::Rect viewportRect) = 0;

		virtual void DrawIndexed(const Ref<const Mesh>& mesh,
			uint32_t subMeshIndex,
			uint32_t baseInstance,
			uint32_t instanceCount) = 0;

		virtual void StartTimer(Ref<GPUTimer> timer) = 0;
		virtual void StopTimer(Ref<GPUTimer> timer) = 0;
	};
}
