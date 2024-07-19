#pragma once

namespace Flare
{
	class Viewport;
	class FrameBuffer;

	class RenderGraphContext
	{
	public:
		RenderGraphContext(const Viewport& viewport, Ref<FrameBuffer> renderTarget)
			: m_Viewport(viewport), m_RenderTarget(renderTarget) {}

		inline const Viewport& GetViewport() const { return m_Viewport; }
		inline Ref<FrameBuffer> GetRenderTarget() const { return m_RenderTarget; }
	private:
		Ref<FrameBuffer> m_RenderTarget = nullptr;
		const Viewport& m_Viewport;
	};
}
