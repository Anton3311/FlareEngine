#pragma once

namespace Flare
{
	class Viewport;
	class FrameBuffer;
	class RenderGraph;
	class RenderGraphResourceManager;

	struct RenderView;
	struct SceneSubmition;

	class RenderGraphContext
	{
	public:
		RenderGraphContext(const Viewport& viewport,
			Ref<FrameBuffer> renderTarget,
			const RenderGraph& renderGraph,
			const RenderGraphResourceManager& resourceManager,
			const SceneSubmition& sceneSubmition,
			const RenderView& view)
			: m_Viewport(viewport),
			m_RenderTarget(renderTarget),
			m_RenderGraph(renderGraph),
			m_SceneSubmition(sceneSubmition),
			m_RenderView(view),
			m_RenderGraphResourceManager(resourceManager) {}

		inline const RenderGraph& GetRenderGraph() const { return m_RenderGraph; }
		inline const RenderGraphResourceManager& GetRenderGraphResourceManager() const { return m_RenderGraphResourceManager; }

		inline const SceneSubmition& GetSceneSubmition() const { return m_SceneSubmition; }
		inline const RenderView& GetRenderView() const { return m_RenderView; }

		inline const Viewport& GetViewport() const { return m_Viewport; }
		inline Ref<FrameBuffer> GetRenderTarget() const { return m_RenderTarget; }
	private:
		Ref<FrameBuffer> m_RenderTarget = nullptr;
		const Viewport& m_Viewport;
		const SceneSubmition& m_SceneSubmition;
		const RenderView& m_RenderView;

		const RenderGraph& m_RenderGraph;
		const RenderGraphResourceManager& m_RenderGraphResourceManager;
	};
}
