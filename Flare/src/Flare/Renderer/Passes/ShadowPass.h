#pragma once

#include "FlareCore/Core.h"
#include "Flare/Renderer/RendererSubmitionQueue.h"
#include "Flare/Renderer/RenderData.h"

#include "Flare/Renderer/RenderGraph/RenderGraphPass.h"

#ifndef FIXED_SHADOW_NEAR_AND_FAR
	#define FIXED_SHADOW_NEAR_AND_FAR 1
#endif

namespace Flare
{
	class CommandBuffer;
	class DescriptorSet;
	class FrameBuffer;
	class ShaderStorageBuffer;
	class Sampler;
	class GPUTimer;
	class UniformBuffer;
	class Material;
	class Mesh;

	class FLARE_API ShadowPass : public RenderGraphPass
	{
	public:
		static constexpr size_t MaxCascades = 4;

		ShadowPass(const RendererSubmitionQueue& opaqueObjects);

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;

		inline const std::vector<uint32_t>& GetVisibleObjects(size_t cascadeIndex) const
		{
			FLARE_CORE_ASSERT(cascadeIndex < MaxCascades);
			return m_VisibleObjects[cascadeIndex];
		}

		inline const RenderView& GetLightView(size_t cascadeIndex) const
		{
			FLARE_CORE_ASSERT(cascadeIndex < MaxCascades);
			return m_LightViews[cascadeIndex];
		}

		inline Ref<UniformBuffer> GetShadowDataBuffer() const { return m_ShadowDataBuffer; }
		inline Ref<Sampler> GetCompareSampler() const { return m_CompareSampler; }
	private:
		struct ShadowData
		{
			float LightSize = 0.0f;
			float LightFar = 0.0f;
			float Bias = 0.0f;
			float NormalBias = 0.0f;

			float FrustumWidth[4] = { 0.0f };
			float CascadeSplits[4] = { 0.0f };

			glm::mat4 LightProjections[4] = { glm::mat4(0.0f) };

			float Resolution = 0.0f;
			float Softness = 0.0f;

			float ShadowFadeStartDistance = 0.0f;
			float MaxShadowDistance = 0.0f;

			int32_t MaxCascadeIndex = 0;

		};

		void CalculateShadowMappingParameters(const RenderGraphContext& context);
		void ComputeShaderProjectionsAndCullObjects(const RenderGraphContext& context);
	private:
		const RendererSubmitionQueue& m_OpaqueObjects;

		ShadowData m_ShadowData;
		Ref<Sampler> m_CompareSampler = nullptr;
		Ref<UniformBuffer> m_ShadowDataBuffer = nullptr;

		RenderView m_LightViews[MaxCascades];
		std::vector<uint32_t> m_VisibleObjects[MaxCascades];
	};
}
