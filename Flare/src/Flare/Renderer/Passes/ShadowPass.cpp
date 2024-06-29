#include "ShadowPass.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/Sampler.h"
#include "Flare/Renderer/GPUTimer.h"

#include "Flare/Math/SIMD.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	ShadowPass::ShadowPass(const RendererSubmitionQueue& opaqueObjects)
		: m_OpaqueObjects(opaqueObjects)
	{
		FLARE_PROFILE_FUNCTION();

		m_ShadowDataBuffer = UniformBuffer::Create(sizeof(ShadowData));

		SamplerSpecifications samplerSpecifications{};
		samplerSpecifications.ComparisonEnabled = true;
		samplerSpecifications.ComparisonFunction = DepthComparisonFunction::Less;
		samplerSpecifications.Filter = TextureFiltering::Linear;
		samplerSpecifications.WrapMode = TextureWrap::Clamp;

		m_CompareSampler = Sampler::Create(samplerSpecifications);
	}

	void ShadowPass::OnRender(const RenderGraphContext& context, Ref<CommandBuffer> commandBuffer)
	{
		if (context.GetViewport().IsShadowMappingEnabled())
		{
			for (auto& visibleObjects : m_VisibleObjects)
				visibleObjects.clear();

			ComputeShaderProjectionsAndCullObjects(context);
		}

		CalculateShadowMappingParameters(context);
	}

	struct CascadeFrustum
	{
		static constexpr size_t LeftIndex = 0;
		static constexpr size_t RightIndex = 1;
		static constexpr size_t TopIndex = 2;
		static constexpr size_t BottomIndex = 3;

		Math::Plane Planes[4];
	};

	struct ShadowMappingParams
	{
		glm::vec3 CameraFrustumCenter;
		float BoundingSphereRadius;
	};

	static void CalculateShadowFrustumParamsAroundCamera(ShadowMappingParams& params,
		glm::vec3 lightDirection,
		const Viewport& viewport,
		float nearPlaneDistance,
		float farPlaneDistance)
	{
		FLARE_PROFILE_FUNCTION();
		const RenderView& camera = viewport.FrameData.Camera;

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
			frustumCorners[i] = viewport.FrameData.Camera.InverseViewProjection * frustumCorners[i];
			frustumCorners[i] /= frustumCorners[i].w;
		}

		Math::Plane farPlane = Math::Plane::TroughPoint(camera.Position + camera.ViewDirection * farPlaneDistance, camera.ViewDirection);
		Math::Plane nearPlane = Math::Plane::TroughPoint(camera.Position + camera.ViewDirection * nearPlaneDistance, camera.ViewDirection);
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

		params.CameraFrustumCenter = frustumCenter;
		params.BoundingSphereRadius = boundingSphereRadius;
	}

	static void CalculateShadowProjectionFrustum(CascadeFrustum* outPlanes, const ShadowMappingParams& params, glm::vec3 lightDirection, const Math::Basis& lightBasis)
	{
		FLARE_PROFILE_FUNCTION();
		outPlanes->Planes[CascadeFrustum::LeftIndex] = Math::Plane::TroughPoint(
			params.CameraFrustumCenter - lightBasis.Right * params.BoundingSphereRadius,
			lightBasis.Right);

		outPlanes->Planes[CascadeFrustum::RightIndex] = Math::Plane::TroughPoint(
			params.CameraFrustumCenter + lightBasis.Right * params.BoundingSphereRadius,
			-lightBasis.Right);

		outPlanes->Planes[CascadeFrustum::TopIndex] = Math::Plane::TroughPoint(
			params.CameraFrustumCenter + lightBasis.Up * params.BoundingSphereRadius,
			-lightBasis.Up);

		outPlanes->Planes[CascadeFrustum::BottomIndex] = Math::Plane::TroughPoint(
			params.CameraFrustumCenter - lightBasis.Up * params.BoundingSphereRadius,
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
		m_ShadowData.FrustumSize = 2.0f * viewport.FrameData.Camera.Near
			* glm::tan(glm::radians(viewport.FrameData.Camera.FOV / 2.0f))
			* viewport.GetAspectRatio();

		for (uint32_t i = 1; i < 4; i++)
			m_ShadowData.CascadeFilterWeights[i] = 1.0f / (m_ShadowData.CascadeFilterWeights[i] / m_ShadowData.CascadeFilterWeights[0]);
		m_ShadowData.CascadeFilterWeights[0] = 1.0f;

		m_ShadowData.MaxShadowDistance = settings.CascadeSplits[settings.Cascades - 1];
		m_ShadowData.ShadowFadeStartDistance = m_ShadowData.MaxShadowDistance - settings.FadeDistance;

		m_ShadowDataBuffer->SetData(&m_ShadowData, sizeof(m_ShadowData), 0);
	}

	void ShadowPass::ComputeShaderProjectionsAndCullObjects(const RenderGraphContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		const Viewport& viewport = context.GetViewport();
		glm::vec3 lightDirection = viewport.FrameData.Light.Direction;
		const Math::Basis& lightBasis = viewport.FrameData.LightBasis;
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();

		ShadowMappingParams perCascadeParams[ShadowSettings::MaxCascades];
		CascadeFrustum cascadeFrustums[ShadowSettings::MaxCascades];

		{
			FLARE_PROFILE_SCOPE("CalculateCascadeFrustum");

			float currentNearPlane = viewport.FrameData.Light.Near;
			for (size_t i = 0; i < shadowSettings.Cascades; i++)
			{
				// 1. Calculate a fit frustum around camera's furstum
				CalculateShadowFrustumParamsAroundCamera(perCascadeParams[i], lightDirection,
					viewport, currentNearPlane,
					shadowSettings.CascadeSplits[i]);

				// 2. Calculate projection frustum planes (except near and far)
				CalculateShadowProjectionFrustum(
					&cascadeFrustums[i],
					perCascadeParams[i],
					lightDirection,
					lightBasis);

				currentNearPlane = shadowSettings.CascadeSplits[i];
			}
		}

		{
			FLARE_PROFILE_SCOPE("DivideIntoGroups");
			for (size_t objectIndex = 0; objectIndex < m_OpaqueObjects.GetSize(); objectIndex++)
			{
				const auto& object = m_OpaqueObjects[objectIndex];
				if (HAS_BIT(object.Flags, MeshRenderFlags::DontCastShadows))
					continue;

				Math::AABB objectAABB = Math::SIMD::TransformAABB(object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds, object.Transform.ToMatrix4x4());
				for (size_t cascadeIndex = 0; cascadeIndex < shadowSettings.Cascades; cascadeIndex++)
				{
					bool intersects = true;
					for (size_t i = 0; i < 4; i++)
					{
						if (!objectAABB.IntersectsOrInFrontOfPlane(cascadeFrustums[cascadeIndex].Planes[i]))
						{
							intersects = false;
							break;
						}
					}

					if (intersects)
						m_VisibleObjects[cascadeIndex].push_back((uint32_t)objectIndex);
				}
			}
		}

		{
			float currentNearPlane = viewport.FrameData.Light.Near;
			for (size_t cascadeIndex = 0; cascadeIndex < shadowSettings.Cascades; cascadeIndex++)
			{
				ShadowMappingParams params = perCascadeParams[cascadeIndex];

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

				// Move shadow map in texel size increaments. in order to avoid shadow edge swimming
				// https://alextardif.com/shadowmapping.html
				float texelsPerUnit = (float)GetShadowMapResolution(Renderer::GetShadowSettings().Quality) / (params.BoundingSphereRadius * 2.0f);

				glm::mat4 view = glm::scale(
					glm::lookAt(
						params.CameraFrustumCenter + lightDirection * nearPlaneDistance,
						params.CameraFrustumCenter, glm::vec3(0.0f, 1.0f, 0.0f)),
					glm::vec3(texelsPerUnit));

				glm::vec4 projectedCenter = view * glm::vec4(params.CameraFrustumCenter, 1.0f);
				projectedCenter.x = glm::round(projectedCenter.x);
				projectedCenter.y = glm::round(projectedCenter.y);

				params.CameraFrustumCenter = glm::inverse(view) * glm::vec4((glm::vec3)projectedCenter, 1.0f);

				view = glm::lookAt(
					params.CameraFrustumCenter + lightDirection * nearPlaneDistance,
					params.CameraFrustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

				glm::mat4 projection;
				
				if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
				{
					projection = glm::orthoRH_ZO(
						-params.BoundingSphereRadius,
						params.BoundingSphereRadius,
						-params.BoundingSphereRadius,
						params.BoundingSphereRadius,
						viewport.FrameData.Light.Near,
						farPlaneDistance - nearPlaneDistance);
				}

				m_LightViews[cascadeIndex].SetViewAndProjection(projection, view);

				m_ShadowData.SceneScale[cascadeIndex] = params.BoundingSphereRadius * glm::sqrt(2.0f);
				m_ShadowData.LightFar = farPlaneDistance - nearPlaneDistance;
				m_ShadowData.CascadeFilterWeights[cascadeIndex] = params.BoundingSphereRadius * 2.0f;
				m_ShadowData.LightProjections[cascadeIndex] = m_LightViews[cascadeIndex].ViewProjection;

				currentNearPlane = shadowSettings.CascadeSplits[cascadeIndex];
			}
		}
	}
}
