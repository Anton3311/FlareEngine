#include "ShadowPass.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/CommandBuffer.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/FrameBuffer.h"
#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/GPUTimer.h"

#include "Flare/Math/SIMD.h"

#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanContext.h"

#include "FlareCore/Profiler/Profiler.h"

namespace Flare
{
	ShadowPass::ShadowPass(const RendererSubmitionQueue& opaqueObjects, Ref<DescriptorSet> primarySet, Ref<DescriptorSet> descriptorSets[MaxCascades])
		: RenderPass(RenderPassQueue::BeforeShadows), m_OpaqueObjects(opaqueObjects), m_PrimarySet(primarySet)
	{
		FLARE_PROFILE_FUNCTION();

		m_ShadowDataBuffer = UniformBuffer::Create(sizeof(ShadowData), 0);
		m_Timer = GPUTimer::Create();

		for (size_t i = 0; i < MaxCascades; i++)
			m_CameraBuffers[i] = UniformBuffer::Create(sizeof(RenderView), 0);

		for (size_t i = 0; i < MaxCascades; i++)
			m_DescriptorSets[i] = descriptorSets[i];

		m_PrimarySet->WriteUniformBuffer(m_ShadowDataBuffer, 2);
		m_PrimarySet->FlushWrites();

		constexpr size_t maxInstanceCount = 1000;
		for (size_t i = 0; i < MaxCascades; i++)
		{
			m_InstanceBuffers[i] = ShaderStorageBuffer::Create(maxInstanceCount * sizeof(InstanceData), 0);

			m_DescriptorSets[i]->WriteUniformBuffer(m_CameraBuffers[i], 0);
			m_DescriptorSets[i]->WriteUniformBuffer(m_ShadowDataBuffer, 2);
			m_DescriptorSets[i]->WriteStorageBuffer(m_InstanceBuffers[i], 3);
			m_DescriptorSets[i]->FlushWrites();
		}
	}

	void ShadowPass::OnRender(RenderingContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		const Viewport& currentViewport = Renderer::GetCurrentViewport();
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();
		
		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();
		std::vector<uint32_t> perCascadeObjects[ShadowSettings::MaxCascades];

		commandBuffer->StartTimer(m_Timer);

		ComputeShaderProjectionsAndCullObjects(perCascadeObjects);
		CalculateShadowMappingParameters();

		uint32_t shadowMapResolution = GetShadowMapResolution(shadowSettings.Quality);
		commandBuffer->SetViewportAndScisors(Math::Rect(0.0f, 0.0f, (float)shadowMapResolution, (float)shadowMapResolution));

		PrepareRenderTargets(commandBuffer);

		for (size_t cascadeIndex = 0; cascadeIndex < shadowSettings.Cascades; cascadeIndex++)
		{
			if (perCascadeObjects[cascadeIndex].size() == 0)
			{
				continue;
			}

#if 0
			{
				FLARE_PROFILE_SCOPE("GroupByMesh");

				// Group objects with same meshes together
				std::sort(perCascadeObjects[cascadeIndex].begin(),
					perCascadeObjects[cascadeIndex].end(),
					[this](uint32_t aIndex, uint32_t bIndex) -> bool
					{
						const auto& a = m_OpaqueObjects[aIndex];
						const auto& b = m_OpaqueObjects[bIndex];

						if (a.Mesh.get() == a.Mesh.get())
							return a.SubMeshIndex < b.SubMeshIndex;

						return (uint64_t)a.Mesh.get() < (uint64_t)b.Mesh.get();
					});
			}
#endif

			Ref<ShaderStorageBuffer> instanceBuffer = m_InstanceBuffers[cascadeIndex];

			{
				FLARE_PROFILE_SCOPE("FillInstanceData");

				m_InstanceDataBuffer.clear();
				for (uint32_t i : perCascadeObjects[cascadeIndex])
				{
					auto& instanceData = m_InstanceDataBuffer.emplace_back();
					const auto& transform = m_OpaqueObjects[i].Transform;
					instanceData.PackedTransform[0] = glm::vec4(transform.RotationScale[0], transform.Translation.x);
					instanceData.PackedTransform[1] = glm::vec4(transform.RotationScale[1], transform.Translation.y);
					instanceData.PackedTransform[2] = glm::vec4(transform.RotationScale[2], transform.Translation.z);
				}
			}

			{
				FLARE_PROFILE_SCOPE("SetInstanceData");
				instanceBuffer->SetData(MemorySpan::FromVector(m_InstanceDataBuffer), 0, commandBuffer);
			}

			DrawCascade((uint32_t)cascadeIndex, commandBuffer, perCascadeObjects[cascadeIndex]);
		}

		commandBuffer->StopTimer(m_Timer);

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			TransitionLayouts(commandBuffer);
		}
	}

	Ref<FrameBuffer> ShadowPass::GetShadowRenderTarget(uint32_t index)
	{
		FLARE_CORE_ASSERT(index < Renderer::GetShadowSettings().Cascades);
		return m_Cascades[index];
	}

	std::optional<float> ShadowPass::GetElapsedTime() const
	{
		return m_Timer->GetElapsedTime();
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

	void ShadowPass::PrepareRenderTargets(const Ref<CommandBuffer>& commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		const ShadowSettings& settings = Renderer::GetShadowSettings();
		uint32_t resolution = GetShadowMapResolution(settings.Quality);
	
		for (uint32_t i = 0; i < (uint32_t)settings.Cascades; i++)
		{
			bool shouldUpdateDescriptor = false;
			if (m_Cascades[i] == nullptr)
			{
				FrameBufferSpecifications specifications{};
				specifications.Width = resolution;
				specifications.Height = resolution;
				specifications.Attachments.push_back({ TextureFormat::Depth32, TextureWrap::Clamp, TextureFiltering::Closest });

				m_Cascades[i] = FrameBuffer::Create(specifications);
				shouldUpdateDescriptor = true;
			}
			else if (m_Cascades[i]->GetSize() != glm::uvec2(resolution, resolution))
			{
				m_Cascades[i]->Resize(resolution, resolution);
				shouldUpdateDescriptor = true;
			}

			if (shouldUpdateDescriptor)
			{
				m_PrimarySet->WriteImage(m_Cascades[i], 0, 28 + i);
			}
		}

		m_PrimarySet->FlushWrites();

		for (uint32_t i = 0; i < (uint32_t)settings.Cascades; i++)
		{
			commandBuffer->ClearDepthAttachment(m_Cascades[i], 1.0f);
		}
	}

	void ShadowPass::CalculateShadowMappingParameters()
	{
		FLARE_PROFILE_FUNCTION();

		const ShadowSettings& settings = Renderer::GetShadowSettings();
		const Viewport& viewport = Renderer::GetCurrentViewport();

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

	void ShadowPass::ComputeShaderProjectionsAndCullObjects(std::vector<uint32_t>* perCascadeObjects)
	{
		FLARE_PROFILE_FUNCTION();

		Viewport& viewport = Renderer::GetCurrentViewport();
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
						perCascadeObjects[cascadeIndex].push_back((uint32_t)objectIndex);
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

				viewport.FrameData.LightView[cascadeIndex].SetViewAndProjection(projection, view);

				m_ShadowData.CascadeFilterWeights[cascadeIndex] = params.BoundingSphereRadius * 2.0f;
				m_ShadowData.LightProjections[cascadeIndex] = viewport.FrameData.LightView[cascadeIndex].ViewProjection;

				currentNearPlane = shadowSettings.CascadeSplits[cascadeIndex];
			}
		}
	}

	void ShadowPass::DrawCascade(uint32_t cascadeIndex, const Ref<CommandBuffer>& commandBuffer, const std::vector<uint32_t>& visibleObjects)
	{
		FLARE_PROFILE_FUNCTION();

		const Viewport& currentViewport = Renderer::GetCurrentViewport();

		commandBuffer->BeginRenderTarget(m_Cascades[cascadeIndex]);

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().GetPrimaryCommandBuffer();

			commandBuffer->SetPrimaryDescriptorSet(m_DescriptorSets[cascadeIndex]);
			commandBuffer->SetSecondaryDescriptorSet(nullptr);
		}

		commandBuffer->ApplyMaterial(Renderer::GetDepthOnlyMaterial());

		m_CameraBuffers[cascadeIndex]->SetData(
			&currentViewport.FrameData.LightView[cascadeIndex],
			sizeof(RenderView), 0);

		Batch batch{};
		for (uint32_t objectIndex : visibleObjects)
		{
			const auto& object = m_OpaqueObjects[objectIndex];
			if (object.Mesh.get() != batch.Mesh.get() || object.SubMeshIndex != batch.SubMesh)
			{
				FlushBatch(commandBuffer, batch);

				batch.BaseInstance += batch.InstanceCount;
				batch.InstanceCount = 0;
				batch.Mesh = object.Mesh;
				batch.SubMesh = object.SubMeshIndex;
			}

			batch.InstanceCount++;
		}

		// Draw remaining objects
		batch.InstanceCount = (uint32_t)visibleObjects.size() - batch.BaseInstance;
		FlushBatch(commandBuffer, batch);

		commandBuffer->EndRenderTarget();
	}

	void ShadowPass::FlushBatch(const Ref<CommandBuffer>& commandBuffer, const Batch& batch)
	{
		FLARE_PROFILE_FUNCTION();

		if (batch.InstanceCount == 0)
			return;

		commandBuffer->DrawIndexed(batch.Mesh, batch.SubMesh, batch.BaseInstance, batch.InstanceCount);
	}

	void ShadowPass::TransitionLayouts(const Ref<CommandBuffer>& commandBuffer)
	{
		FLARE_PROFILE_FUNCTION();
		Ref<VulkanCommandBuffer> vulkanCommandBuffer = VulkanContext::GetInstance().GetPrimaryCommandBuffer();
		VkImage images[4] = { VK_NULL_HANDLE };

		for (uint32_t i = 0; i < (uint32_t)Renderer::GetShadowSettings().Cascades; i++)
		{
			images[i] = As<VulkanFrameBuffer>(m_Cascades[i])->GetAttachmentImage(0);
		}

		TextureFormat format = m_Cascades[0]->GetAttachment(0)->GetFormat();
		vulkanCommandBuffer->DepthImagesBarrier(Span(images, Renderer::GetShadowSettings().Cascades),
			HasStencilComponent(format),
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_ACCESS_NONE,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}
