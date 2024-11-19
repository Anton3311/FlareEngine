#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#include "FlareECS/Entity/ComponentInitializer.h"

namespace Flare
{
	class DescriptorSet;
	class GPUBuffer;

	struct FLARE_API CulledGeometry
	{
		FLARE_COMPONENT;
		FLARE_NONCOPYABLE(CulledGeometry);

		CulledGeometry();

		CulledGeometry(CulledGeometry&&) = default;
		CulledGeometry& operator=(CulledGeometry&&) = default;

		struct InstanceData
		{
			glm::vec4 PackedTransform[3];
		};

		struct FLARE_API GPUFrameResources
		{
			FLARE_NONCOPYABLE(GPUFrameResources);

			GPUFrameResources() = default;

			GPUFrameResources(GPUFrameResources&&) = default;
			GPUFrameResources& operator=(GPUFrameResources&&) = default;

			~GPUFrameResources();

			Ref<GPUBuffer> InstanceBuffer = nullptr;
			Ref<DescriptorSet> InstanceBufferDescriptor = nullptr;
		};

		std::vector<uint32_t> VisibleObjects;
		std::vector<InstanceData> InstanceDataBuffer;
		std::vector<GPUFrameResources> FrameResources;
	};

	class World;
	class GeometryCullingPass : public RenderGraphPass
	{
	public:
		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		static void CullGeometry(const RenderGraphContext& context, std::vector<uint32_t>& culledGeometry);
	};
}
