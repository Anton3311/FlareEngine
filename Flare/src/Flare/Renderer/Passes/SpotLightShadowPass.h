#pragma once

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"
#include "Flare/Renderer/RenderGraph/RenderGraphResourceManager.h"
#include "Flare/Renderer/Passes/GeometryCullingPass.h"

namespace Flare
{
	class DescriptorSet;
	class GPUBuffer;
	class Material;

	struct SpotLightShadowsEntry
	{
		glm::mat4 ViewProjection;
		float Bias;
		float NormalBias;
		uint32_t UVTransform;
	};

	struct SpotLightShadowsSpecifications
	{
		uint32_t MaxLightCount = 4;
		uint32_t SizeLog2 = 10; // 1024
	};

	struct SpotLightCulledGeometryRange
	{
		uint32_t Start;
		uint32_t Count;
	};

	struct SpotLightCulledGeometryBatch
	{
		Ref<const Mesh> GeometryMesh = nullptr;
		uint32_t TransformBufferOffset = 0;
		uint32_t Count = 0;
	};

	struct SpotLightTile
	{
		glm::ivec2 Position;
		uint32_t SizeLog2;
		SpotLightCulledGeometryRange CulledBatches;
	};

	class SpotLightShadowPass : public RenderGraphPass
	{
	public:
		SpotLightShadowPass(RenderGraphTextureId shadowMap,
			Ref<Material> perspectiveDepthOnly,
			const SpotLightShadowsSpecifications& specifications);
		~SpotLightShadowPass();

		void OnPrepare(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;
	private:
		const SpotLightShadowsEntry* PrepareShadowEntries(const RenderGraphContext& context);

		size_t CullGeometryForLight(const RenderGraphContext& context, size_t lightIndex, const SpotLightShadowsEntry& shadows);
		void CullGeometry(const RenderGraphContext& context, const SpotLightShadowsEntry* shadowEntries);
	private:
		struct PerLightCameraResources
		{
			Ref<DescriptorSet> CameraDescriptorSet = nullptr;
			Ref<GPUBuffer> CameraBuffer = nullptr;
		};

		struct TransformBufferResources
		{
			Ref<DescriptorSet> Set = nullptr;
			Ref<GPUBuffer> Buffer = nullptr;
		};

		SpotLightShadowsSpecifications m_Specifications;

		bool m_HasSpotLight = false;
		RenderGraphTextureId m_ShadowMap;
		Ref<Material> m_PerspectiveDepthOnly = nullptr;

		std::vector<PerLightCameraResources> m_PerLightCameras;
		std::vector<TransformBufferResources> m_TransformBuffers;

		std::vector<PackedTransform> m_CulledGeometryTransforms;
		std::vector<SpotLightCulledGeometryBatch> m_CulledBatches;
		std::vector<SpotLightTile> m_Tiles;
	};
}
