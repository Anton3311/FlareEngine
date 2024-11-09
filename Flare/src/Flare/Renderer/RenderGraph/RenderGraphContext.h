#pragma once

#include <glm/glm.hpp>

#include "FlareECS/Entity/Entity.h"

namespace Flare
{
	class RenderGraph;
	class RenderGraphResourceManager;

	struct RenderView;
	struct SceneSubmition;
	class World;

	class RenderGraphContext
	{
	public:
		RenderGraphContext(Entity viewportEntity,
			World& renderWorld,
			glm::uvec2 renderAreaSize,
			const RenderGraph& renderGraph,
			const RenderGraphResourceManager& resourceManager,
			const SceneSubmition& sceneSubmition,
			const RenderView& view)
			: RenderAreaSize(renderAreaSize),
			ViewportEntity(viewportEntity),
			RenderWorld(renderWorld),
			m_RenderGraph(renderGraph),
			m_SceneSubmition(sceneSubmition),
			m_RenderView(view),
			m_RenderGraphResourceManager(resourceManager) {}

		inline const RenderGraph& GetRenderGraph() const { return m_RenderGraph; }
		inline const RenderGraphResourceManager& GetRenderGraphResourceManager() const { return m_RenderGraphResourceManager; }

		inline const SceneSubmition& GetSceneSubmition() const { return m_SceneSubmition; }
		inline const RenderView& GetRenderView() const { return m_RenderView; }
	public:
		const glm::uvec2 RenderAreaSize = glm::uvec2(0, 0);
		const Entity ViewportEntity;
		World& RenderWorld;
	private:
		const SceneSubmition& m_SceneSubmition;
		const RenderView& m_RenderView;

		const RenderGraph& m_RenderGraph;
		const RenderGraphResourceManager& m_RenderGraphResourceManager;
	};
}
