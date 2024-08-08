#pragma once

#include <glm/glm.hpp>

namespace Flare
{
	class Viewport;
	class RenderGraph;
	class RenderGraphResourceManager;

	struct RenderView;
	struct SceneSubmition;

	class RenderGraphContext
	{
	public:
		RenderGraphContext(const Viewport& viewport,
			glm::uvec2 renderAreaSize,
			const RenderGraph& renderGraph,
			const RenderGraphResourceManager& resourceManager,
			const SceneSubmition& sceneSubmition,
			const RenderView& view)
			: m_Viewport(viewport),
			RenderAreaSize(renderAreaSize),
			m_RenderGraph(renderGraph),
			m_SceneSubmition(sceneSubmition),
			m_RenderView(view),
			m_RenderGraphResourceManager(resourceManager) {}

		inline const RenderGraph& GetRenderGraph() const { return m_RenderGraph; }
		inline const RenderGraphResourceManager& GetRenderGraphResourceManager() const { return m_RenderGraphResourceManager; }

		inline const SceneSubmition& GetSceneSubmition() const { return m_SceneSubmition; }
		inline const RenderView& GetRenderView() const { return m_RenderView; }

		inline const Viewport& GetViewport() const { return m_Viewport; }
	public:
		const glm::uvec2 RenderAreaSize = glm::uvec2(0, 0);
	private:
		const Viewport& m_Viewport;
		const SceneSubmition& m_SceneSubmition;
		const RenderView& m_RenderView;

		const RenderGraph& m_RenderGraph;
		const RenderGraphResourceManager& m_RenderGraphResourceManager;
	};
}
