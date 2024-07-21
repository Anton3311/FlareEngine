#include "ShadowPass.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/Sampler.h"
#include "Flare/Renderer/GPUTimer.h"
#include "Flare/Renderer/SceneSubmition.h"

#include "Flare/Math/SIMD.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	ShadowPass::ShadowPass()
	{
		FLARE_PROFILE_FUNCTION();

		SamplerSpecifications samplerSpecifications{};
		samplerSpecifications.ComparisonEnabled = true;
		samplerSpecifications.ComparisonFunction = DepthComparisonFunction::Less;
		samplerSpecifications.Filter = TextureFiltering::Linear;
		samplerSpecifications.WrapMode = TextureWrap::Clamp;

		m_CompareSampler = Sampler::Create(samplerSpecifications);
	}

	void ShadowPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		if (context.GetViewport().IsShadowMappingEnabled())
		{
			for (ShadowCascadeData& cascadeData : m_CascadeData)
			{
				cascadeData.Batches.clear();
				cascadeData.PartiallyVisible.clear();
			}

			m_FilteredTransforms.clear();
			m_VisibleSubMeshRanges.clear();

			ComputeShaderProjectionsAndCullObjects(context);
		}

		CalculateShadowMappingParameters(context);
	}

	static void CalculateShadowFrustumParamsAroundCamera(ShadowCascadeData& cascadeData,
		const RenderView& cameraView,
		glm::vec3 lightDirection,
		const Viewport& viewport,
		float nearPlaneDistance,
		float farPlaneDistance)
	{
		FLARE_PROFILE_FUNCTION();

		std::array<glm::vec4, 8> frustumCorners =
		{
			glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f),
			glm::vec4(1.0f,  1.0f, 0.0f, 1.0f),
			glm::vec4(-1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f, -1.0f, 1.0f, 1.0f),
			glm::vec4(-1.0f,  1.0f, 1.0f, 1.0f),
			glm::vec4(1.0f,  1.0f, 1.0f, 1.0f),
		};

		for (size_t i = 0; i < frustumCorners.size(); i++)
		{
			frustumCorners[i] = cameraView.InverseViewProjection * frustumCorners[i];
			frustumCorners[i] /= frustumCorners[i].w;
		}

		Math::Plane farPlane = Math::Plane::TroughPoint(cameraView.Position + cameraView.ViewDirection * farPlaneDistance, cameraView.ViewDirection);
		Math::Plane nearPlane = Math::Plane::TroughPoint(cameraView.Position + cameraView.ViewDirection * nearPlaneDistance, cameraView.ViewDirection);
		for (size_t i = 0; i < frustumCorners.size() / 2; i++)
		{
			Math::Ray ray;
			ray.Origin = frustumCorners[i];
			ray.Direction = frustumCorners[i + 4] - frustumCorners[i];

			frustumCorners[i + 4] = glm::vec4(ray.Origin + ray.Direction * Math::IntersectPlane(farPlane, ray), 0.0f);
		}

		for (size_t i = 0; i < frustumCorners.size() / 2; i++)
		{
			Math::Ray ray;
			ray.Origin = frustumCorners[i + 4];
			ray.Direction = frustumCorners[i] - frustumCorners[i + 4];

			frustumCorners[i] = glm::vec4(ray.Origin + ray.Direction * Math::IntersectPlane(nearPlane, ray), 0.0f);
		}

		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (size_t i = 0; i < frustumCorners.size(); i++)
			frustumCenter += (glm::vec3)frustumCorners[i];
		frustumCenter /= frustumCorners.size();

		float boundingSphereRadius = 0.0f;
		for (size_t i = 0; i < frustumCorners.size(); i++)
			boundingSphereRadius = glm::max(boundingSphereRadius, glm::distance(frustumCenter, (glm::vec3)frustumCorners[i]));

		cascadeData.BoundingSphereCenter = frustumCenter;
		cascadeData.BoundingSphereRadius = boundingSphereRadius;
	}

	static void CalculateShadowProjectionFrustum(ShadowCascadeData& cascadeData, glm::vec3 lightDirection, const Math::Basis& lightBasis)
	{
		FLARE_PROFILE_FUNCTION();

		constexpr size_t LeftIndex = 0;
		constexpr size_t RightIndex = 1;
		constexpr size_t TopIndex = 2;
		constexpr size_t BottomIndex = 3;

		cascadeData.FrustumPlanes[LeftIndex] = Math::Plane::TroughPoint(
			cascadeData.BoundingSphereCenter - lightBasis.Right * cascadeData.BoundingSphereRadius,
			lightBasis.Right);

		cascadeData.FrustumPlanes[RightIndex] = Math::Plane::TroughPoint(
			cascadeData.BoundingSphereCenter + lightBasis.Right * cascadeData.BoundingSphereRadius,
			-lightBasis.Right);

		cascadeData.FrustumPlanes[TopIndex] = Math::Plane::TroughPoint(
			cascadeData.BoundingSphereCenter + lightBasis.Up * cascadeData.BoundingSphereRadius,
			-lightBasis.Up);

		cascadeData.FrustumPlanes[BottomIndex] = Math::Plane::TroughPoint(
			cascadeData.BoundingSphereCenter - lightBasis.Up * cascadeData.BoundingSphereRadius,
			lightBasis.Up);
	}

	void ShadowPass::CalculateShadowMappingParameters(const RenderGraphContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		const ShadowSettings& settings = Renderer::GetShadowSettings();
		const Viewport& viewport = context.GetViewport();

		m_ShadowData.Bias = settings.Bias;
		m_ShadowData.NormalBias = settings.NormalBias;
		m_ShadowData.LightSize = settings.LightSize;
		m_ShadowData.Resolution = (float)GetShadowMapResolution(settings.Quality);
		m_ShadowData.Softness = settings.Softness;

		for (size_t i = 0; i < 4; i++)
			m_ShadowData.CascadeSplits[i] = settings.CascadeSplits[i];

		m_ShadowData.MaxCascadeIndex = settings.Cascades - 1;

		m_ShadowData.MaxShadowDistance = settings.CascadeSplits[settings.Cascades - 1];
		m_ShadowData.ShadowFadeStartDistance = m_ShadowData.MaxShadowDistance - settings.FadeDistance;

		context.GetViewport().GlobalResources.ShadowDataBuffer->SetData(&m_ShadowData, sizeof(m_ShadowData), 0);
	}

	enum class CullResult
	{
		NotVisible,
		PartiallyVisible,
		FullyVisible,
	};

	inline static CullResult CullAABB(const Math::AABB& aabb, const Math::Plane* planes, const Math::Compact3DTransform& transform)
	{
		Math::AABB transformedAABB = Math::SIMD::TransformAABB(aabb, transform.ToMatrix4x4());

		__m128 min = _mm_loadu_ps(glm::value_ptr(glm::vec4(aabb.Min, 0.0f)));
		__m128 max = _mm_loadu_ps(glm::value_ptr(glm::vec4(aabb.Max, 0.0f)));

		// Calculate AABB extents
		__m128 extents = _mm_sub_ps(max, min);

		// Calculate AABB center
		float scale = 0.5f;
		__m128 halfScale = _mm_load_ps1(&scale);
		__m128 center = _mm_mul_ps(_mm_add_ps(min, max), halfScale);

		size_t inFrontCount = 0;
		bool intersects = true;
		for (size_t i = 0; i < 4; i++)
		{
			Math::Plane plane = planes[i];

			__m128 planeNormal = _mm_loadu_ps(glm::value_ptr(glm::vec4(plane.Normal, 0.0f)));
			float projectedDistance = Math::SIMD::Dot(Math::SIMD::Abs(planeNormal), extents);
			float signedDistance = glm::abs(Math::SIMD::Dot(planeNormal, center));

			inFrontCount += (projectedDistance <= signedDistance) ? 1 : 0;

			if (-projectedDistance > signedDistance) // No intersection and not in front
				return CullResult::NotVisible;
		}

		return inFrontCount == 4 ? CullResult::FullyVisible : CullResult::PartiallyVisible;
	}

	void ShadowPass::ComputeShaderProjectionsAndCullObjects(const RenderGraphContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		const Viewport& viewport = context.GetViewport();
		const DirectionalLightSubmition& directionalLight = context.GetSceneSubmition().DirectionalLight;
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();

		constexpr float LIGHT_NEAR = 0.1f;

		{
			FLARE_PROFILE_SCOPE("CalculateCascadeFrustum");

			// TODO: Shouldn't be hard coded
			float currentNearPlane = LIGHT_NEAR;
			for (size_t i = 0; i < shadowSettings.Cascades; i++)
			{
				// 1. Calculate a fit frustum around camera's furstum
				CalculateShadowFrustumParamsAroundCamera(m_CascadeData[i],
					context.GetRenderView(),
					directionalLight.Direction,
					viewport, currentNearPlane,
					shadowSettings.CascadeSplits[i]);

				// 2. Calculate projection frustum planes (except near and far)
				CalculateShadowProjectionFrustum(
					m_CascadeData[i],
					directionalLight.Direction,
					directionalLight.LightBasis);

				currentNearPlane = shadowSettings.CascadeSplits[i];

				m_ShadowData.FrustumWidth[i] = m_CascadeData->BoundingSphereRadius * 2.0f;
			}
		}

		FilterSubmitions(context);

		{
			float currentNearPlane = LIGHT_NEAR;
			for (size_t cascadeIndex = 0; cascadeIndex < shadowSettings.Cascades; cascadeIndex++)
			{
				float nearPlaneDistance = 0;
				float farPlaneDistance = 0;
#if !FIXED_SHADOW_NEAR_AND_FAR
				{
					FLARE_PROFILE_SCOPE("ExtendFrustums");
					// 3. Extend near and far planes

					Math::Plane nearPlane = Math::Plane::TroughPoint(params.CameraFrustumCenter, -lightDirection);
					Math::Plane farPlane = Math::Plane::TroughPoint(params.CameraFrustumCenter, lightDirection);


					for (uint32_t objectIndex : perCascadeObjects[cascadeIndex])
					{
						const auto& object = m_OpaqueObjects[objectIndex];
						Math::AABB objectAABB = object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds.Transformed(object.Transform);

						glm::vec3 center = objectAABB.GetCenter();
						glm::vec3 extents = objectAABB.Max - center;

						float projectedDistance = glm::dot(glm::abs(nearPlane.Normal), extents);
						nearPlaneDistance = glm::max(nearPlaneDistance, nearPlane.Distance(center) + projectedDistance);
						farPlaneDistance = glm::max(farPlaneDistance, farPlane.Distance(center) + projectedDistance);
					}

				}
				
				nearPlaneDistance = -nearPlaneDistance;
				farPlaneDistance = farPlaneDistance;
#else
				const float fixedPlaneDistance = 500.0f;
				nearPlaneDistance = -fixedPlaneDistance;
				farPlaneDistance = fixedPlaneDistance;
#endif

				ShadowCascadeData& cascadeData = m_CascadeData[cascadeIndex];

				// Move shadow map in texel size increaments. in order to avoid shadow edge swimming
				// https://alextardif.com/shadowmapping.html
				float texelsPerUnit = (float)GetShadowMapResolution(Renderer::GetShadowSettings().Quality) / (cascadeData.BoundingSphereRadius * 2.0f);

				glm::mat4 view = glm::scale(
					glm::lookAt(
						cascadeData.BoundingSphereCenter + directionalLight.Direction * nearPlaneDistance,
						cascadeData.BoundingSphereCenter, glm::vec3(0.0f, 1.0f, 0.0f)),
					glm::vec3(texelsPerUnit));

				glm::vec4 projectedCenter = view * glm::vec4(cascadeData.BoundingSphereCenter, 1.0f);
				projectedCenter.x = glm::round(projectedCenter.x);
				projectedCenter.y = glm::round(projectedCenter.y);

				cascadeData.BoundingSphereCenter = glm::inverse(view) * glm::vec4((glm::vec3)projectedCenter, 1.0f);

				view = glm::lookAt(
					cascadeData.BoundingSphereCenter + directionalLight.Direction * nearPlaneDistance,
					cascadeData.BoundingSphereCenter, glm::vec3(0.0f, 1.0f, 0.0f));

				glm::mat4 projection;
				
				if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
				{
					projection = glm::orthoRH_ZO(
						-cascadeData.BoundingSphereRadius,
						cascadeData.BoundingSphereRadius,
						-cascadeData.BoundingSphereRadius,
						cascadeData.BoundingSphereRadius,
						LIGHT_NEAR,
						farPlaneDistance - nearPlaneDistance);
				}

				m_CascadeData[cascadeIndex].View.SetViewAndProjection(projection, view);

				m_ShadowData.LightFar = farPlaneDistance - nearPlaneDistance;
				m_ShadowData.LightProjections[cascadeIndex] = m_CascadeData[cascadeIndex].View.ViewProjection;

				currentNearPlane = shadowSettings.CascadeSplits[cascadeIndex];
			}
		}
	}

	void ShadowPass::FilterSubmitions(const RenderGraphContext& context)
	{
		FLARE_PROFILE_FUNCTION();
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();
		const auto& submitedBatches = context.GetSceneSubmition().OpaqueGeometrySubmitions.GetShadowPassBatches();

		for (size_t batchIndex = 0; batchIndex < submitedBatches.size(); batchIndex++)
		{
			const RendererSubmitionQueue::ShadowPassBatch& batch = submitedBatches[batchIndex];
			for (size_t cascadeIndex = 0; cascadeIndex < (size_t)shadowSettings.Cascades; cascadeIndex++)
			{
				ShadowCascadeData& cascadeData = m_CascadeData[cascadeIndex];
				FilteredShadowPassBatch filteredBatch{};
				filteredBatch.Mesh = batch.Mesh;
				filteredBatch.FirstEntryIndex = (uint32_t)m_FilteredTransforms.size();

				for (size_t submitionIndex = 0; submitionIndex < batch.Submitions.size(); submitionIndex++)
				{
					CullResult result = CullAABB(
						filteredBatch.Mesh->GetBounds(),
						cascadeData.FrustumPlanes,
						batch.Submitions[submitionIndex].Transform);

					if (result == CullResult::PartiallyVisible && filteredBatch.Mesh->GetSubMeshes().size() == 1)
						result = CullResult::FullyVisible;

					if (result == CullResult::FullyVisible)
					{
						m_FilteredTransforms.push_back(batch.Submitions[submitionIndex].Transform);
						filteredBatch.Count++;
					}
					else if (result == CullResult::PartiallyVisible)
					{
						PartiallyVisibleMesh partiallyVisibleMesh{};
						partiallyVisibleMesh.Mesh = filteredBatch.Mesh;
						partiallyVisibleMesh.Transform = batch.Submitions[submitionIndex].Transform;

						CullSubMeshes(partiallyVisibleMesh, batch.Submitions[submitionIndex].Transform, cascadeData.FrustumPlanes);

						if (partiallyVisibleMesh.SubMeshRangeCount > 0)
						{
							cascadeData.PartiallyVisible.push_back(partiallyVisibleMesh);
						}
					}
				}

				if (filteredBatch.Count > 0)
				{
					cascadeData.Batches.push_back(filteredBatch);
				}
			}
		}
	}

	void ShadowPass::CullSubMeshes(PartiallyVisibleMesh& mesh, const Math::Compact3DTransform& transform, const Math::Plane* frustumPlanes)
	{
		FLARE_PROFILE_FUNCTION();

		mesh.FirstSubMeshRange = (uint32_t)m_VisibleSubMeshRanges.size();

		VisibleSubMeshRange currentRange{};
		currentRange.Start = UINT32_MAX;
		currentRange.Count = 0;

		const auto& subMeshes = mesh.Mesh->GetSubMeshes();
		uint32_t subMeshCount = (uint32_t)subMeshes.size();
		for (uint32_t i = 0; i < subMeshCount; i++)
		{
			CullResult result = CullAABB(subMeshes[i].Bounds, frustumPlanes, transform);
			if (result == CullResult::NotVisible)
				continue;

			if (currentRange.Start == UINT32_MAX || i != currentRange.GetEnd() + 1)
			{
				if (currentRange.Count > 0)
				{
					m_VisibleSubMeshRanges.push_back(currentRange);
					mesh.SubMeshRangeCount++;
				}

				currentRange.Start = i;
				currentRange.Count = 1;
			}
			else
			{
				currentRange.Count++;
			}
		}

		if (currentRange.Count > 0)
		{
			m_VisibleSubMeshRanges.push_back(currentRange);
			mesh.SubMeshRangeCount++;
		}
	}
}
