#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraph.h"

namespace Flare
{
	class RenderGraphBuilder;
	class VulkanRenderPass;
	class VulkanFrameBuffer;
	class FLARE_API VulkanRenderGraph : public RenderGraph
	{
	public:
		VulkanRenderGraph(const Viewport& viewport);

		void Execute(Ref<CommandBuffer> commandBuffer, const SceneSubmition& sceneSubmition, const RenderView& view) override;
	private:
		size_t GetRenderTargetIndex(size_t nodeIndex) const;

		void ExecuteLayoutTransitions(Ref<CommandBuffer> commandBuffer, LayoutTransitionsRange range);
		void CreateRenderTargets(uint32_t frameIndex);

		void SelectVulkanRenderPasses(const RenderGraphBuilder& renderGraphBuilder);
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

			Ref<VulkanRenderPass> VulkanRenderPassHandle = nullptr;
		};

		std::vector<Ref<VulkanFrameBuffer>> m_RenderTargets;
		std::vector<NodeData> m_NodeData;
	};
}
