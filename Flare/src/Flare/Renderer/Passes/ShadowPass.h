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

	struct FilteredShadowPassBatch
	{
		Ref<const Mesh> Mesh = nullptr;
		uint32_t FirstEntryIndex = 0;
		uint32_t Count = 0;
	};

	struct PartiallyVisibleMesh
	{
		Ref<const Mesh> Mesh = nullptr;
		Math::Compact3DTransform Transform;
		uint32_t FirstSubMeshRange = 0;
		uint32_t SubMeshRangeCount = 0;
	};

	struct VisibleSubMeshRange
	{
		inline uint32_t GetEnd() const { return Start + Count - 1; }

		uint32_t Start = 0;
		uint32_t Count = 0;
	};

	struct ShadowCascadeData
	{
		RenderView View;
		Math::Plane FrustumPlanes[4];

		glm::vec3 BoundingSphereCenter = glm::vec3(0.0f);
		float BoundingSphereRadius = 0.0f;

		std::vector<FilteredShadowPassBatch> Batches;
		std::vector<PartiallyVisibleMesh> PartiallyVisible;
	};

	class FLARE_API ShadowPass : public RenderGraphPass
	{
	public:
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

		static constexpr size_t MaxCascades = 4;

		ShadowPass(const RendererSubmitionQueue& opaqueObjects);

		void OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer) override;

		inline Ref<Sampler> GetCompareSampler() const { return m_CompareSampler; }
		inline const ShadowCascadeData& GetCascadeData(size_t index) const { return m_CascadeData[index]; }
		inline const std::vector<Math::Compact3DTransform>& GetFilteredTransforms() const { return m_FilteredTransforms; }
		inline const std::vector<VisibleSubMeshRange>& GetVisibleSubMeshIndices() const { return m_VisibleSubMeshRanges; }
	private:
		void CalculateShadowMappingParameters(const RenderGraphContext& context);
		void ComputeShaderProjectionsAndCullObjects(const RenderGraphContext& context);
		void FilterSubmitions();

		void CullSubMeshes(PartiallyVisibleMesh& mesh,
			const Math::Compact3DTransform& transform,
			const Math::Plane* frustumPlanes);
	private:
		const RendererSubmitionQueue& m_OpaqueObjects;

		ShadowData m_ShadowData;
		Ref<Sampler> m_CompareSampler = nullptr;

		ShadowCascadeData m_CascadeData[MaxCascades];
		std::vector<Math::Compact3DTransform> m_FilteredTransforms;
		std::vector<VisibleSubMeshRange> m_VisibleSubMeshRanges;
	};
}
