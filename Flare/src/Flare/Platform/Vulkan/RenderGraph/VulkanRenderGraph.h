#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

namespace Flare
{
	class VulkanRenderPass;
	class VulkanFrameBuffer;
	class FLARE_API VulkanRenderGraph : public RenderGraph
	{
	public:
		VulkanRenderGraph(const Viewport& viewport);

		void Execute(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view) override;

		size_t GetRenderTargetIndex(size_t nodeIndex) const;
	private:
		void ExecuteLayoutTransitions(Ref<CommandBuffer> commandBuffer, LayoutTransitionsRange range);
		void CreateRenderTargets();
	protected:
		void OnPrepare() override;
		void OnTexturesResize() override;
		void OnClear() override;
		void OnBuild() override;
	private:
		struct NodeData
		{
			static constexpr uint32_t INVALID_TARGET_INDEX = UINT32_MAX;

			LayoutTransitionsRange ExplicitTransitions;
			uint32_t RenderTargetHandleIndex = INVALID_TARGET_INDEX;
		};

		std::vector<Ref<VulkanFrameBuffer>> m_RenderTargets;
		std::vector<NodeData> m_NodeData;
	};
}
