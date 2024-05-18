#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RenderPass.h"
#include "Flare/Renderer/RendererSubmitionQueue.h"
#include "Flare/Renderer/RenderData.h"

#ifndef FIXED_SHADOW_NEAR_AND_FAR
	#define FIXED_SHADOW_NEAR_AND_FAR 1
#endif

namespace Flare
{
	class CommandBuffer;
	class DescriptorSet;
	class FrameBuffer;
	class ShaderStorageBuffer;
	class GPUTimer;
	class UniformBuffer;
	class Material;
	class Mesh;

	class FLARE_API ShadowPass : public RenderPass
	{
	public:
		static constexpr size_t MaxCascades = 4;

		ShadowPass(const RendererSubmitionQueue& opaqueObjects,
			Ref<DescriptorSet> primarySet,
			Ref<DescriptorSet> descriptorSets[MaxCascades]);

		void OnRender(RenderingContext& context) override;

		Ref<FrameBuffer> GetShadowRenderTarget(uint32_t index);
	private:
		struct InstanceData
		{
			glm::vec4 PackedTransform[3];
		};

		struct Batch
		{
			Ref<const Mesh> Mesh = nullptr;
			uint32_t SubMesh = 0;
			uint32_t BaseInstance = 0;
			uint32_t InstanceCount = 0;
		};

		struct ShadowData
		{
			float Bias = 0.0f;
			float FrustumSize = 0.0f;
			float LightSize = 0.0f;

			int32_t MaxCascadeIndex = 0;

			float CascadeSplits[4] = { 0.0f };
			float CascadeFilterWeights[4] = { 0.0f };

			glm::mat4 LightProjections[4] = { glm::mat4(0.0f) };

			float Resolution = 0.0f;
			float Softness = 0.0f;

			float ShadowFadeStartDistance = 0.0f;
			float MaxShadowDistance = 0.0f;

			float NormalBias = 0.0f;
		};

		void PrepareRenderTargets(const Ref<CommandBuffer>& commandBuffer);
		void CalculateShadowMappingParameters();
		void ComputeShaderProjectionsAndCullObjects(std::vector<uint32_t>* perCascadeObjects);
		void DrawCascade(uint32_t cascadeIndex, const Ref<CommandBuffer>& commandBuffer, const std::vector<uint32_t>& visibleObjects);
		void FlushBatch(const Ref<CommandBuffer>& commandBuffer, const Batch& batch);
		void TransitionLayouts(const Ref<CommandBuffer>& commandBuffer);
	private:
		Ref<GPUTimer> m_Timer = nullptr;
		Ref<DescriptorSet> m_PrimarySet = nullptr;

		ShadowData m_ShadowData;
		Ref<UniformBuffer> m_ShadowDataBuffer = nullptr;

		std::vector<InstanceData> m_InstanceDataBuffer;

		const RendererSubmitionQueue& m_OpaqueObjects;
		Ref<FrameBuffer> m_Cascades[MaxCascades] = { nullptr };
		Ref<ShaderStorageBuffer> m_InstanceBuffers[MaxCascades] = { nullptr };
		Ref<UniformBuffer> m_CameraBuffers[MaxCascades] = { nullptr };
		Ref<DescriptorSet> m_DescriptorSets[MaxCascades] = { nullptr };
	};
}
