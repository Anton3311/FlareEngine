#include "Renderer.h"

#include "Flare/Renderer/RendererPrimitives.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Math/Transform.h"
#include "Flare/Math/SIMD.h"

#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/GPUTimer.h"

#include "Flare/Project/Project.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanGPUTimer.h"

namespace Flare
{
#define FIXED_SHADOW_NEAR_AND_FAR 1

	FLARE_IMPL_TYPE(ShadowSettings);

	struct InstanceData
	{
		glm::mat4 Transform;
		int32_t EntityIndex;
		int32_t Padding[3];
	};

	struct RenderPasses
	{
		std::vector<Ref<RenderPass>> BeforeShadowsPasses;
		std::vector<Ref<RenderPass>> BeforeOpaqueGeometryPasses;
		std::vector<Ref<RenderPass>> AfterOpaqueGeometryPasses;
		std::vector<Ref<RenderPass>> PostProcessingPasses;
	};

	struct ShadowData
	{
		float Bias;
		float FrustumSize;
		float LightSize;

		int32_t MaxCascadeIndex;

		float CascadeSplits[4];

		glm::mat4 LightProjection[4];

		float Resolution;
		float Softness;

		float ShadowFadeStartDistance;
		float MaxShadowDistance;

		float NormalBias;
	};

	struct InstancingMesh
	{
		inline void Reset()
		{
			Mesh = nullptr;
			SubMeshIndex = UINT32_MAX;
		}

		Ref<const Mesh> Mesh = nullptr;
		uint32_t SubMeshIndex = UINT32_MAX;
	};

	struct DecalData
	{
		glm::mat4 Transform;
		int32_t EntityIndex;
		Ref<const Material> Material;
	};

	struct RendererData
	{
		Viewport* MainViewport = nullptr;
		Viewport* CurrentViewport = nullptr;

		Ref<UniformBuffer> CameraBuffer = nullptr;
		Ref<UniformBuffer> LightBuffer = nullptr;

		Ref<Texture> WhiteTexture = nullptr;
		Ref<Texture> DefaultNormalMap = nullptr;

		Ref<Material> ErrorMaterial = nullptr;
		Ref<Material> DepthOnlyMeshMaterial = nullptr;
		
		RenderPasses Passes;
		RendererStatistics Statistics;

		RendererSubmitionQueue OpaqueQueue;
		std::vector<uint32_t> CulledObjectIndices;
		std::vector<DecalData> Decals;

		// Instancing

		std::vector<InstanceData> InstanceDataBuffer;
		uint32_t MaxInstances = 1024 * 24;

		Ref<ShaderStorageBuffer> InstancesShaderBuffer = nullptr;
		std::vector<DrawIndirectCommandSubMeshData> IndirectDrawData;

		// Shadows
		InstancingMesh CurrentInstancingMesh;
		Ref<FrameBuffer> ShadowsRenderTarget[ShadowSettings::MaxCascades] = { nullptr };
		Ref<ShaderStorageBuffer> ShadowPassInstanceBuffers[ShadowSettings::MaxCascades] = { nullptr };
		Ref<UniformBuffer> ShadowPassCameraBuffers[ShadowSettings::MaxCascades] = { nullptr };
		Ref<VulkanDescriptorSet> PerCascadeDescriptorSets[ShadowSettings::MaxCascades] = { nullptr };

		ShadowSettings ShadowMappingSettings;
		Ref<UniformBuffer> ShadowDataBuffer = nullptr;

		// Lighting
		
		std::vector<PointLightData> PointLights;
		Ref<ShaderStorageBuffer> PointLightsShaderBuffer = nullptr;
		std::vector<SpotLightData> SpotLights;
		Ref<ShaderStorageBuffer> SpotLightsShaderBuffer = nullptr;

		Ref<GPUTimer> ShadowPassTimer = nullptr;
		Ref<GPUTimer> GeometryPassTimer = nullptr;

		Ref<VulkanDescriptorSet> PrimaryDescriptorSet = nullptr;
		Ref<VulkanDescriptorSet> PrimaryDescriptorSetWithoutShadows = nullptr;
		Ref<VulkanDescriptorSetPool> PrimaryDescriptorPool = nullptr;
	};
	
	RendererData s_RendererData;

	void Renderer::ReloadShaders()
	{
		std::optional<AssetHandle> errorShaderHandle = ShaderLibrary::FindShader("Error");
		if (errorShaderHandle && AssetManager::IsAssetHandleValid(*errorShaderHandle))
		{
			s_RendererData.ErrorMaterial = Material::Create(AssetManager::GetAsset<Shader>(*errorShaderHandle));
			s_RendererData.OpaqueQueue.m_ErrorMaterial = s_RendererData.ErrorMaterial;
		}
		else
			FLARE_CORE_ERROR("Renderer: Failed to find Error shader");

		std::optional<AssetHandle> depthOnlyMeshShaderHandle = ShaderLibrary::FindShader("MeshDepthOnly");
		if (depthOnlyMeshShaderHandle && AssetManager::IsAssetHandleValid(*depthOnlyMeshShaderHandle))
			s_RendererData.DepthOnlyMeshMaterial= Material::Create(AssetManager::GetAsset<Shader>(*depthOnlyMeshShaderHandle));
		else
			FLARE_CORE_ERROR("Renderer: Failed to find MeshDepthOnly shader");
	}

	void Renderer::Initialize()
	{
		size_t maxPointLights = 32;
		size_t maxSpotLights = 32;

		s_RendererData.CameraBuffer = UniformBuffer::Create(sizeof(RenderView), 0);
		s_RendererData.LightBuffer = UniformBuffer::Create(sizeof(LightData), 1);
		s_RendererData.ShadowDataBuffer = UniformBuffer::Create(sizeof(ShadowData), 2);

		s_RendererData.InstancesShaderBuffer = ShaderStorageBuffer::Create(s_RendererData.MaxInstances * sizeof(InstanceData), 3);
		s_RendererData.InstancesShaderBuffer->SetDebugName("InstanceDataBuffer");

		s_RendererData.PointLightsShaderBuffer = ShaderStorageBuffer::Create(maxPointLights * sizeof(PointLightData), 4);
		s_RendererData.PointLightsShaderBuffer->SetDebugName("PointLightsDataBuffer");

		s_RendererData.SpotLightsShaderBuffer = ShaderStorageBuffer::Create(maxSpotLights * sizeof(SpotLightData), 5);
		s_RendererData.SpotLightsShaderBuffer->SetDebugName("SpotLightsDataBuffer");

		for (uint32_t i = 0; i < ShadowSettings::MaxCascades; i++)
		{
			s_RendererData.ShadowPassCameraBuffers[i] = UniformBuffer::Create(sizeof(RenderView), 0);
		}

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			s_RendererData.ShadowPassInstanceBuffers[0] = s_RendererData.InstancesShaderBuffer;
			for (uint32_t i = 0; i < ShadowSettings::MaxCascades; i++)
			{
				s_RendererData.ShadowPassInstanceBuffers[i] = ShaderStorageBuffer::Create(s_RendererData.MaxInstances * sizeof(InstanceData), 3);
				s_RendererData.ShadowPassInstanceBuffers[i]->SetDebugName(fmt::format("Cascade{}.InstanceDataBuffer", i));
			}
		}
		else
		{
			s_RendererData.ShadowPassInstanceBuffers[0] = s_RendererData.InstancesShaderBuffer;
			s_RendererData.ShadowPassInstanceBuffers[1] = s_RendererData.InstancesShaderBuffer;
			s_RendererData.ShadowPassInstanceBuffers[2] = s_RendererData.InstancesShaderBuffer;
			s_RendererData.ShadowPassInstanceBuffers[3] = s_RendererData.InstancesShaderBuffer;
		}

		s_RendererData.ShadowPassTimer = GPUTimer::Create();
		s_RendererData.GeometryPassTimer = GPUTimer::Create();

		s_RendererData.ShadowMappingSettings.Quality = ShadowQuality::Medium;
		s_RendererData.ShadowMappingSettings.Bias = 0.015f;
		s_RendererData.ShadowMappingSettings.LightSize = 0.009f;

		s_RendererData.ShadowMappingSettings.Cascades = s_RendererData.ShadowMappingSettings.MaxCascades;
		s_RendererData.ShadowMappingSettings.CascadeSplits[0] = 15.0f;
		s_RendererData.ShadowMappingSettings.CascadeSplits[1] = 25.0f;
		s_RendererData.ShadowMappingSettings.CascadeSplits[2] = 50.0f;
		s_RendererData.ShadowMappingSettings.CascadeSplits[3] = 100.0f;

		{
			uint32_t whiteTextureData = 0xffffffff;
			s_RendererData.WhiteTexture = Texture::Create(1, 1, &whiteTextureData, TextureFormat::RGBA8);
		}

		{
			uint32_t pixel = 0xffff8080;
			s_RendererData.DefaultNormalMap = Texture::Create(1, 1, &pixel, TextureFormat::RGBA8);
		}

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			VkDescriptorSetLayoutBinding bindings[6 + 4] = {};
			// Camera
			bindings[0].binding = 0;
			bindings[0].descriptorCount = 1;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[0].pImmutableSamplers = nullptr;
			bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			// Light
			bindings[1].binding = 1;
			bindings[1].descriptorCount = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[1].pImmutableSamplers = nullptr;
			bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			
			// Shadow data
			bindings[2].binding = 2;
			bindings[2].descriptorCount = 1;
			bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[2].pImmutableSamplers = nullptr;
			bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			// Instance data
			bindings[3].binding = 3;
			bindings[3].descriptorCount = 1;
			bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			bindings[3].pImmutableSamplers = nullptr;
			bindings[3].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

			// Point lights
			bindings[4].binding = 4;
			bindings[4].descriptorCount = 1;
			bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			bindings[4].pImmutableSamplers = nullptr;
			bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			// Spot lights
			bindings[5].binding = 5;
			bindings[5].descriptorCount = 1;
			bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			bindings[5].pImmutableSamplers = nullptr;
			bindings[5].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			// Shadow cascades
			for (uint32_t i = 0; i < 4; i++)
			{
				bindings[i + 6].binding = i + 28;
				bindings[i + 6].descriptorCount = 1;
				bindings[i + 6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				bindings[i + 6].pImmutableSamplers = nullptr;
				bindings[i + 6].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			s_RendererData.PrimaryDescriptorPool = CreateRef<VulkanDescriptorSetPool>(2 + ShadowSettings::MaxCascades, Span(bindings, 6 + 4));
			s_RendererData.PrimaryDescriptorSet = s_RendererData.PrimaryDescriptorPool->AllocateSet();
			s_RendererData.PrimaryDescriptorSet->SetDebugName("PrimarySet");

			s_RendererData.PrimaryDescriptorSetWithoutShadows = s_RendererData.PrimaryDescriptorPool->AllocateSet();
			s_RendererData.PrimaryDescriptorSetWithoutShadows->SetDebugName("PrimarySetWithoutShadows");

			// Setup primary descriptor set
			s_RendererData.PrimaryDescriptorSet->WriteUniformBuffer(s_RendererData.CameraBuffer, 0);
			s_RendererData.PrimaryDescriptorSet->WriteUniformBuffer(s_RendererData.LightBuffer, 1);
			s_RendererData.PrimaryDescriptorSet->WriteUniformBuffer(s_RendererData.ShadowDataBuffer, 2);
			s_RendererData.PrimaryDescriptorSet->WriteStorageBuffer(s_RendererData.InstancesShaderBuffer, 3);
			s_RendererData.PrimaryDescriptorSet->WriteStorageBuffer(s_RendererData.PointLightsShaderBuffer, 4);
			s_RendererData.PrimaryDescriptorSet->WriteStorageBuffer(s_RendererData.SpotLightsShaderBuffer, 5);

			for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
			{
				s_RendererData.PrimaryDescriptorSet->WriteTexture(s_RendererData.WhiteTexture, (uint32_t)(28 + i));
			}

			s_RendererData.PrimaryDescriptorSet->FlushWrites();

			// Setup primary descriptor set (without shadows)
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteUniformBuffer(s_RendererData.CameraBuffer, 0);
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteUniformBuffer(s_RendererData.LightBuffer, 1);
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteUniformBuffer(s_RendererData.ShadowDataBuffer, 2);
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteStorageBuffer(s_RendererData.InstancesShaderBuffer, 3);
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteStorageBuffer(s_RendererData.PointLightsShaderBuffer, 4);
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteStorageBuffer(s_RendererData.SpotLightsShaderBuffer, 5);

			for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
			{
				s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteTexture(s_RendererData.WhiteTexture, (uint32_t)(28 + i));
			}

			s_RendererData.PrimaryDescriptorSetWithoutShadows->FlushWrites();
			
			// Setup per cascade descriptor sets
			for (uint32_t i = 0; i < ShadowSettings::MaxCascades; i++)
			{
				Ref<VulkanDescriptorSet> set = s_RendererData.PrimaryDescriptorPool->AllocateSet();
				set->WriteUniformBuffer(s_RendererData.ShadowPassCameraBuffers[i], 0);
				set->WriteUniformBuffer(s_RendererData.LightBuffer, 1);
				set->WriteUniformBuffer(s_RendererData.ShadowDataBuffer, 2);
				set->WriteStorageBuffer(s_RendererData.ShadowPassInstanceBuffers[i], 3);
				set->WriteStorageBuffer(s_RendererData.PointLightsShaderBuffer, 4);
				set->WriteStorageBuffer(s_RendererData.SpotLightsShaderBuffer, 5);

				for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
				{
					set->WriteTexture(s_RendererData.WhiteTexture, (uint32_t)(28 + i));
				}

				set->FlushWrites();
				set->SetDebugName(fmt::format("Cascade{}.PrimaryDescriptorSet", i));
				s_RendererData.PerCascadeDescriptorSets[i] = set;
			}
		}

		Project::OnProjectOpen.Bind(ReloadShaders);
	}

	void Renderer::Shutdown()
	{
		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			s_RendererData.PrimaryDescriptorPool->ReleaseSet(s_RendererData.PrimaryDescriptorSet);
		}

		s_RendererData = {};
	}

	const RendererStatistics& Renderer::GetStatistics()
	{
		return s_RendererData.Statistics;
	}

	void Renderer::ClearStatistics()
	{
		s_RendererData.Statistics.DrawCallsCount = 0;
		s_RendererData.Statistics.DrawCallsSavedByInstancing = 0;
		s_RendererData.Statistics.ObjectsCulled = 0;
		s_RendererData.Statistics.ObjectsSubmitted = 0;
		s_RendererData.Statistics.GeometryPassTime = 0.0f;
		s_RendererData.Statistics.ShadowPassTime = 0.0f;
	}

	void Renderer::SetMainViewport(Viewport& viewport)
	{
		s_RendererData.MainViewport = &viewport;
	}

	void Renderer::SetCurrentViewport(Viewport& viewport)
	{
		s_RendererData.CurrentViewport = &viewport;
	}

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

	struct CascadeFrustum
	{
		static constexpr size_t LeftIndex = 0;
		static constexpr size_t RightIndex = 1;
		static constexpr size_t TopIndex = 2;
		static constexpr size_t BottomIndex = 3;

		Math::Plane Planes[4];
	};

	static void CalculateShadowProjectionFrustum(CascadeFrustum* outPlanes, const ShadowMappingParams& params, glm::vec3 lightDirection, const Math::Basis& lightBasis)
	{
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

	static void CalculateShadowProjections(std::vector<uint32_t> perCascadeObjects[ShadowSettings::MaxCascades])
	{
		FLARE_PROFILE_FUNCTION();

		Viewport* viewport = s_RendererData.CurrentViewport;
		glm::vec3 lightDirection = viewport->FrameData.Light.Direction;
		const Math::Basis& lightBasis = viewport->FrameData.LightBasis;
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();

		ShadowMappingParams perCascadeParams[ShadowSettings::MaxCascades];
		CascadeFrustum cascadeFrustums[ShadowSettings::MaxCascades];

		{
			FLARE_PROFILE_SCOPE("CalculateCascadeFrustum");

			float currentNearPlane = viewport->FrameData.Light.Near;
			for (size_t i = 0; i < shadowSettings.Cascades; i++)
			{
				// 1. Calculate a fit frustum around camera's furstum
				CalculateShadowFrustumParamsAroundCamera(perCascadeParams[i], lightDirection,
					*viewport, currentNearPlane,
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
			for (size_t objectIndex = 0; objectIndex < s_RendererData.OpaqueQueue.GetSize(); objectIndex++)
			{
				const auto& object = s_RendererData.OpaqueQueue[objectIndex];
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
			FLARE_PROFILE_SCOPE("ExtendFrustums");
			float currentNearPlane = viewport->FrameData.Light.Near;
			for (size_t cascadeIndex = 0; cascadeIndex < shadowSettings.Cascades; cascadeIndex++)
			{
				ShadowMappingParams params = perCascadeParams[cascadeIndex];

				float nearPlaneDistance = 0;
				float farPlaneDistance = 0;
#if !FIXED_SHADOW_NEAR_AND_FAR

				// 3. Extend near and far planes

				Math::Plane nearPlane = Math::Plane::TroughPoint(params.CameraFrustumCenter, -lightDirection);
				Math::Plane farPlane = Math::Plane::TroughPoint(params.CameraFrustumCenter, lightDirection);


				for (uint32_t objectIndex : perCascadeObjects[cascadeIndex])
				{
					const RenderableObject& object = s_RendererData.Queue[objectIndex];
					Math::AABB objectAABB = object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds.Transformed(object.Transform);

					glm::vec3 center = objectAABB.GetCenter();
					glm::vec3 extents = objectAABB.Max - center;

					float projectedDistance = glm::dot(glm::abs(nearPlane.Normal), extents);
					nearPlaneDistance = glm::max(nearPlaneDistance, nearPlane.Distance(center) + projectedDistance);
					farPlaneDistance = glm::max(farPlaneDistance, farPlane.Distance(center) + projectedDistance);
				}
				
				nearPlaneDistance = -nearPlaneDistance;
				farPlaneDistance = farPlaneDistance;
#else
				const float fixedPlaneDistance = 500.0f;
				nearPlaneDistance = -fixedPlaneDistance;
				farPlaneDistance = fixedPlaneDistance;
#endif

				// Move shadow map in texel size increaments. in order ot avoid shadow edge swimming
				// https://alextardif.com/shadowmapping.html
				float texelsPerUnit = (float)GetShadowMapResolution(s_RendererData.ShadowMappingSettings.Quality) / (params.BoundingSphereRadius * 2.0f);

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
				
				if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
				{
					projection = glm::ortho(
						-params.BoundingSphereRadius,
						params.BoundingSphereRadius,
						-params.BoundingSphereRadius,
						params.BoundingSphereRadius,
						viewport->FrameData.Light.Near,
						farPlaneDistance - nearPlaneDistance);
				}
				else
				{
					projection = glm::orthoRH_ZO(
						-params.BoundingSphereRadius,
						params.BoundingSphereRadius,
						-params.BoundingSphereRadius,
						params.BoundingSphereRadius,
						viewport->FrameData.Light.Near,
						farPlaneDistance - nearPlaneDistance);
				}

				viewport->FrameData.LightView[cascadeIndex].SetViewAndProjection(projection, view);
				currentNearPlane = shadowSettings.CascadeSplits[cascadeIndex];
			}
		}
	}

	static void CalculateShadowMappingParams()
	{
		FLARE_PROFILE_FUNCTION();

		ShadowData shadowData;
		shadowData.Bias = s_RendererData.ShadowMappingSettings.Bias;
		shadowData.NormalBias = s_RendererData.ShadowMappingSettings.NormalBias;
		shadowData.LightSize = s_RendererData.ShadowMappingSettings.LightSize;
		shadowData.Resolution = (float)GetShadowMapResolution(s_RendererData.ShadowMappingSettings.Quality);
		shadowData.Softness = s_RendererData.ShadowMappingSettings.Softness;

		for (size_t i = 0; i < 4; i++)
			shadowData.CascadeSplits[i] = s_RendererData.ShadowMappingSettings.CascadeSplits[i];

		for (size_t i = 0; i < 4; i++)
			shadowData.LightProjection[i] = s_RendererData.CurrentViewport->FrameData.LightView[i].ViewProjection;

		shadowData.MaxCascadeIndex = s_RendererData.ShadowMappingSettings.Cascades - 1;
		shadowData.FrustumSize = 2.0f * s_RendererData.CurrentViewport->FrameData.Camera.Near
			* glm::tan(glm::radians(s_RendererData.CurrentViewport->FrameData.Camera.FOV / 2.0f))
			* s_RendererData.CurrentViewport->GetAspectRatio();

		shadowData.MaxShadowDistance = shadowData.CascadeSplits[s_RendererData.ShadowMappingSettings.Cascades - 1];
		shadowData.ShadowFadeStartDistance = shadowData.MaxShadowDistance - s_RendererData.ShadowMappingSettings.FadeDistance;

		s_RendererData.ShadowDataBuffer->SetData(&shadowData, sizeof(shadowData), 0);
	}

	void Renderer::BeginScene(Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		s_RendererData.CurrentViewport = &viewport;
		s_RendererData.OpaqueQueue.m_CameraPosition = viewport.FrameData.Camera.Position;

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();

		{
			FLARE_PROFILE_SCOPE("UpdateLightUniformBuffer");
			s_RendererData.CurrentViewport->FrameData.Light.PointLightsCount = (uint32_t)s_RendererData.PointLights.size();
			s_RendererData.CurrentViewport->FrameData.Light.SpotLightsCount = (uint32_t)s_RendererData.SpotLights.size();
			s_RendererData.LightBuffer->SetData(&viewport.FrameData.Light, sizeof(viewport.FrameData.Light), 0);
		}

		{
			FLARE_PROFILE_SCOPE("UpdateCameraUniformBuffer");
			const auto& spec = viewport.RenderTarget->GetSpecifications();
			viewport.FrameData.Camera.ViewportSize = glm::ivec2(spec.Width, spec.Height);
			s_RendererData.CameraBuffer->SetData(&viewport.FrameData.Camera, sizeof(RenderView), 0);
		}

		{
			FLARE_PROFILE_SCOPE("UploadPointLightsData");
			s_RendererData.PointLightsShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.PointLights), 0, commandBuffer);
		}

		{
			FLARE_PROFILE_SCOPE("UploadSpotLightsData");
			s_RendererData.SpotLightsShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.SpotLights), 0, commandBuffer);
		}

		if (s_RendererData.ShadowMappingSettings.Enabled)
		{
			FLARE_PROFILE_SCOPE("ResizeShadowBuffers");
			uint32_t size = (uint32_t)GetShadowMapResolution(s_RendererData.ShadowMappingSettings.Quality);

			bool rewriteDescriptorSet = !s_RendererData.ShadowMappingSettings.Enabled;
			if (s_RendererData.ShadowsRenderTarget[0] == nullptr)
			{
				FrameBufferSpecifications shadowMapSpecs;
				shadowMapSpecs.Width = size;
				shadowMapSpecs.Height = size;
				shadowMapSpecs.Attachments = { { FrameBufferTextureFormat::Depth, TextureWrap::Clamp, TextureFiltering::Linear } };

				for (size_t i = 0; i < 4; i++)
					s_RendererData.ShadowsRenderTarget[i] = FrameBuffer::Create(shadowMapSpecs);

				rewriteDescriptorSet = true;
			}
			else
			{
				auto& shadowMapSpecs = s_RendererData.ShadowsRenderTarget[0]->GetSpecifications();
				if (shadowMapSpecs.Width != size || shadowMapSpecs.Height != size)
				{
					for (size_t i = 0; i < 4; i++)
						s_RendererData.ShadowsRenderTarget[i]->Resize(size, size);

					rewriteDescriptorSet = true;
				}
			}

			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan && rewriteDescriptorSet)
			{
				for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
				{
					s_RendererData.PrimaryDescriptorSet->WriteFrameBufferAttachment(s_RendererData.ShadowsRenderTarget[i], 0, (uint32_t)(28 + i));
				}

				s_RendererData.PrimaryDescriptorSet->FlushWrites();
			}

		}

		for (size_t i = 0; i < s_RendererData.ShadowMappingSettings.Cascades; i++)
		{
			commandBuffer->ClearDepthAttachment(s_RendererData.ShadowsRenderTarget[i], 1.0f);
		}

		viewport.RenderTarget->Bind();

		{
			// Generate camera frustum planes
			FLARE_PROFILE_SCOPE("CalculateFrustumPlanes");

			auto& frameData = s_RendererData.CurrentViewport->FrameData;
			frameData.CameraFrustumPlanes.SetFromViewAndProjection(
				frameData.Camera.View,
				frameData.Camera.InverseViewProjection,
				frameData.Camera.ViewDirection);
		}
	}

	static void ApplyMaterial(const Ref<const Material>& materail)
	{
		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();
		commandBuffer->ApplyMaterial(materail);
		commandBuffer->SetViewportAndScisors(Math::Rect(glm::vec2(0.0f), (glm::vec2)s_RendererData.CurrentViewport->RenderTarget->GetSize()));
	}

	static bool CompareRenderableObjects(uint32_t aIndex, uint32_t bIndex)
	{
		const auto& a = s_RendererData.OpaqueQueue[aIndex];
		const auto& b = s_RendererData.OpaqueQueue[bIndex];

		return a.SortKey < b.SortKey;

		// TODO: group objects based on material and mesh, then sort by distance

#if 0
		if ((uint64_t)a.Material->Handle < (uint64_t)b.Material->Handle)
			return true;

		if (a.Material->Handle == b.Material->Handle)
		{
			if (a.Mesh->Handle == b.Mesh->Handle)
			{
			}

			if ((uint64_t)a.Mesh->Handle < (uint64_t)b.Mesh->Handle)
				return true;
		}

		return false;
#endif
	}

	static void PerformFrustumCulling()
	{
		FLARE_PROFILE_FUNCTION();

		Math::AABB objectAABB;

		const FrustumPlanes& planes = s_RendererData.CurrentViewport->FrameData.CameraFrustumPlanes;
		for (size_t i = 0; i < s_RendererData.OpaqueQueue.GetSize(); i++)
		{
			const auto& object = s_RendererData.OpaqueQueue[i];
			objectAABB = Math::SIMD::TransformAABB(object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds, object.Transform.ToMatrix4x4());

			bool intersects = true;
			for (size_t i = 0; i < planes.PlanesCount; i++)
			{
				if (!objectAABB.IntersectsOrInFrontOfPlane(planes.Planes[i]))
				{
					intersects = false;
					break;
				}
			}

			if (intersects)
			{
				s_RendererData.CulledObjectIndices.push_back((uint32_t)i);
			}
		}
	}

	void Renderer::Flush()
	{
		FLARE_PROFILE_FUNCTION();

		s_RendererData.Statistics.ObjectsSubmitted += (uint32_t)s_RendererData.OpaqueQueue.GetSize();

		if (s_RendererData.ShadowMappingSettings.Enabled && s_RendererData.CurrentViewport->ShadowMappingEnabled)
		{
			ExecuteShadowPass();
		}

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().GetPrimaryCommandBuffer();
			VkImage images[4] = { VK_NULL_HANDLE };

			for (int32_t i = 0; i < s_RendererData.ShadowMappingSettings.Cascades; i++)
			{
				images[i] = As<VulkanFrameBuffer>(s_RendererData.ShadowsRenderTarget[i])->GetAttachmentImage(0);
			}

			commandBuffer->DepthImagesBarrier(Span(images, s_RendererData.ShadowMappingSettings.Cascades), true,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
				VK_ACCESS_NONE,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		{
			FLARE_PROFILE_SCOPE("PrepareViewport");

			s_RendererData.CurrentViewport->RenderTarget->Bind();
			RenderCommand::SetViewport(0, 0, s_RendererData.CurrentViewport->GetSize().x, s_RendererData.CurrentViewport->GetSize().y);

			s_RendererData.CameraBuffer->Bind();
		}

		ExecuteGeomertyPass();

#if 0
		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			ExecuteDecalsPass();
		}
#endif

		s_RendererData.InstanceDataBuffer.clear();
		s_RendererData.CulledObjectIndices.clear();
		s_RendererData.OpaqueQueue.m_Buffer.clear();
	}

	void Renderer::ExecuteGeomertyPass()
	{
		{
			FLARE_PROFILE_SCOPE("BeforeGeometryPasses");
			ExecuteRenderPasses(s_RendererData.Passes.BeforeOpaqueGeometryPasses);
		}

		FLARE_PROFILE_FUNCTION();

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().GetPrimaryCommandBuffer();
			Ref<VulkanFrameBuffer> renderTarget = As<VulkanFrameBuffer>(s_RendererData.CurrentViewport->RenderTarget);
			
			if (s_RendererData.ShadowMappingSettings.Enabled)
			{
				commandBuffer->SetPrimaryDescriptorSet(s_RendererData.PrimaryDescriptorSet);
			}
			else
			{
				commandBuffer->SetPrimaryDescriptorSet(s_RendererData.PrimaryDescriptorSetWithoutShadows);
			}

			commandBuffer->SetSecondaryDescriptorSet(nullptr);
		}

		s_RendererData.CulledObjectIndices.clear();
		PerformFrustumCulling();
		s_RendererData.Statistics.ObjectsCulled += (uint32_t)(s_RendererData.OpaqueQueue.GetSize() - s_RendererData.CulledObjectIndices.size());

		{
			FLARE_PROFILE_SCOPE("Sort");
			std::sort(s_RendererData.CulledObjectIndices.begin(), s_RendererData.CulledObjectIndices.end(), CompareRenderableObjects);
		}

		if (RendererAPI::GetAPI() == RendererAPI::API::OpenGL)
		{
			if (s_RendererData.ShadowMappingSettings.Enabled)
			{
				for (size_t i = 0; i < 4; i++)
					s_RendererData.ShadowsRenderTarget[i]->BindAttachmentTexture(0, 28 + (uint32_t)i);
			}
			else
			{
				for (size_t i = 0; i < 4; i++)
					s_RendererData.WhiteTexture->Bind(28 + (uint32_t)i);
			}
		}

		Ref<const Material> currentMaterial = nullptr;
		s_RendererData.InstanceDataBuffer.clear();

		{
			FLARE_PROFILE_SCOPE("FillInstacesData");
			for (uint32_t objectIndex : s_RendererData.CulledObjectIndices)
			{
				auto& instanceData = s_RendererData.InstanceDataBuffer.emplace_back();
				instanceData.Transform = s_RendererData.OpaqueQueue[objectIndex].Transform.ToMatrix4x4();
				instanceData.EntityIndex = s_RendererData.OpaqueQueue[objectIndex].EntityIndex;
			}
		}

		{
			FLARE_PROFILE_SCOPE("SetIntancesData");
			s_RendererData.InstancesShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.InstanceDataBuffer), 0, commandBuffer);
		}

		commandBuffer->StartTimer(s_RendererData.GeometryPassTimer);
		commandBuffer->BeginRenderTarget(s_RendererData.CurrentViewport->RenderTarget);

		uint32_t baseInstance = 0;
		for (uint32_t currentInstance = 0; currentInstance < (uint32_t)s_RendererData.CulledObjectIndices.size(); currentInstance++)
		{
			uint32_t objectIndex = s_RendererData.CulledObjectIndices[currentInstance];
			const auto& object = s_RendererData.OpaqueQueue[objectIndex];

			if (s_RendererData.CurrentInstancingMesh.Mesh.get() != object.Mesh.get()
				|| s_RendererData.CurrentInstancingMesh.SubMeshIndex != object.SubMeshIndex)
			{
				FlushInstances(currentInstance - baseInstance, baseInstance);
				baseInstance = currentInstance;

				s_RendererData.CurrentInstancingMesh.Mesh = object.Mesh;
				s_RendererData.CurrentInstancingMesh.SubMeshIndex = object.SubMeshIndex;
			}

			if (object.Material.get() != currentMaterial.get())
			{
				FlushInstances(currentInstance - baseInstance, baseInstance);
				baseInstance = currentInstance;

				currentMaterial = object.Material;

				ApplyMaterial(object.Material);
			}
		}

		FlushInstances((uint32_t)s_RendererData.CulledObjectIndices.size() - baseInstance, baseInstance);
		s_RendererData.CurrentInstancingMesh.Reset();

		commandBuffer->EndRenderTarget();
		commandBuffer->StopTimer(s_RendererData.GeometryPassTimer);
		
		{
			FLARE_PROFILE_SCOPE("AfterGeometryPasses");
			ExecuteRenderPasses(s_RendererData.Passes.AfterOpaqueGeometryPasses);
		}
	}

	void Renderer::ExecuteDecalsPass()
	{
		FLARE_PROFILE_FUNCTION();
		if (s_RendererData.Decals.size() == 0)
			return;

		s_RendererData.InstanceDataBuffer.clear();

		{
			FLARE_PROFILE_SCOPE("SortByMaterial");
			std::sort(s_RendererData.Decals.begin(), s_RendererData.Decals.end(), [](const DecalData& a, const DecalData& b) -> bool
			{
				return (uint64_t)a.Material->Handle < (uint64_t)b.Material->Handle;
			});
		}

		{
			FLARE_PROFILE_SCOPE("FillInstancesData");
			for (const DecalData& decal : s_RendererData.Decals)
			{
				auto& instance = s_RendererData.InstanceDataBuffer.emplace_back();
				instance.Transform = decal.Transform;
				instance.EntityIndex = decal.EntityIndex;
			}
		}

		{
			FLARE_PROFILE_SCOPE("SetInstancesData");
			s_RendererData.InstancesShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.InstanceDataBuffer));
		}

		s_RendererData.CurrentViewport->RenderTarget->BindAttachmentTexture(s_RendererData.CurrentViewport->DepthAttachmentIndex, 1);

		uint32_t baseInstance = 0;
		uint32_t instanceIndex = 0;

		Ref<const Material> currentMaterial = s_RendererData.Decals[0].Material;
		Ref<const Mesh> cubeMesh = RendererPrimitives::GetCube();

		for (; instanceIndex < (uint32_t)s_RendererData.Decals.size(); instanceIndex++)
		{
			if (s_RendererData.Decals[instanceIndex].Material.get() != currentMaterial.get())
			{
				ApplyMaterial(currentMaterial);

				RenderCommand::DrawInstancesIndexed(cubeMesh, 0, instanceIndex - baseInstance, baseInstance);
				baseInstance = instanceIndex;

				currentMaterial = s_RendererData.Decals[instanceIndex].Material;
			}
		}

		if (instanceIndex != baseInstance)
		{
			FLARE_CORE_ASSERT(currentMaterial->GetShader());
			ApplyMaterial(currentMaterial);

			RenderCommand::DrawInstancesIndexed(cubeMesh, 0, instanceIndex - baseInstance, baseInstance);
		}

		s_RendererData.Decals.clear();
		s_RendererData.InstanceDataBuffer.clear();
	}

	void Renderer::ExecuteShadowPass()
	{
		{
			FLARE_PROFILE_SCOPE("BeforeShadowsPasses");
			ExecuteRenderPasses(s_RendererData.Passes.BeforeShadowsPasses);
		}

		FLARE_PROFILE_FUNCTION();

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();
		commandBuffer->StartTimer(s_RendererData.ShadowPassTimer);

		std::vector<uint32_t> perCascadeObjects[ShadowSettings::MaxCascades];

		CalculateShadowProjections(perCascadeObjects);
		CalculateShadowMappingParams();

		for (size_t cascadeIndex = 0; cascadeIndex < s_RendererData.ShadowMappingSettings.Cascades; cascadeIndex++)
		{
			const FrameBufferSpecifications& shadowMapSpecs = s_RendererData.ShadowsRenderTarget[cascadeIndex]->GetSpecifications();

			// Fill instancing buffer
			Ref<ShaderStorageBuffer> instanceBuffer = s_RendererData.ShadowPassInstanceBuffers[cascadeIndex];

			{
				FLARE_PROFILE_SCOPE("GroupByMesh");

				// Group objects with same meshes together
				std::sort(perCascadeObjects[cascadeIndex].begin(), perCascadeObjects[cascadeIndex].end(), [](uint32_t aIndex, uint32_t bIndex) -> bool
				{
					const auto& a = s_RendererData.OpaqueQueue[aIndex];
					const auto& b = s_RendererData.OpaqueQueue[bIndex];

					if (a.Mesh.get() == a.Mesh.get())
						return a.SubMeshIndex < b.SubMeshIndex;

					return (uint64_t)a.Mesh.get() < (uint64_t)b.Mesh.get();
				});
			}

			{
				FLARE_PROFILE_SCOPE("FillInstanceData");

				// Fill instances buffer
				s_RendererData.InstanceDataBuffer.clear();
				for (uint32_t i : perCascadeObjects[cascadeIndex])
				{
					auto& instanceData = s_RendererData.InstanceDataBuffer.emplace_back();
					instanceData.Transform = s_RendererData.OpaqueQueue[i].Transform.ToMatrix4x4();
				}
			}

			{
				FLARE_PROFILE_SCOPE("SetInstancesData");
				instanceBuffer->SetData(MemorySpan::FromVector(s_RendererData.InstanceDataBuffer), 0, commandBuffer);
			}

			// Render

			commandBuffer->BeginRenderTarget(s_RendererData.ShadowsRenderTarget[cascadeIndex]);

			if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
			{
				Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().GetPrimaryCommandBuffer();

				commandBuffer->SetPrimaryDescriptorSet(s_RendererData.PerCascadeDescriptorSets[cascadeIndex]);
				commandBuffer->SetSecondaryDescriptorSet(nullptr);
			}

			commandBuffer->ApplyMaterial(s_RendererData.DepthOnlyMeshMaterial);
			commandBuffer->SetViewportAndScisors(Math::Rect(0.0f, 0.0f, shadowMapSpecs.Width, shadowMapSpecs.Height));

			s_RendererData.ShadowPassCameraBuffers[cascadeIndex]->SetData(
				&s_RendererData.CurrentViewport->FrameData.LightView[cascadeIndex],
				sizeof(s_RendererData.CurrentViewport->FrameData.LightView[cascadeIndex]), 0);

			s_RendererData.ShadowPassCameraBuffers[cascadeIndex]->Bind();
			s_RendererData.ShadowsRenderTarget[cascadeIndex]->Bind();
			s_RendererData.CurrentInstancingMesh.Reset();

			s_RendererData.IndirectDrawData.clear();
			s_RendererData.CurrentInstancingMesh.Reset();

			{
				FLARE_PROFILE_SCOPE("ExecuteDrawCalls");

				uint32_t baseInstance = 0;
				uint32_t currentInstance = 0;
				for (uint32_t objectIndex : perCascadeObjects[cascadeIndex])
				{
					const auto& queued = s_RendererData.OpaqueQueue[objectIndex];
					if (queued.Mesh.get() != s_RendererData.CurrentInstancingMesh.Mesh.get())
					{
						FlushShadowPassInstances(baseInstance);

						baseInstance = currentInstance;

						s_RendererData.CurrentInstancingMesh.Mesh = queued.Mesh;
						auto& command = s_RendererData.IndirectDrawData.emplace_back();
						command.InstancesCount = 1;
						command.SubMeshIndex = queued.SubMeshIndex;
					}
					else
					{
						if (s_RendererData.IndirectDrawData.size() > 0 && s_RendererData.IndirectDrawData.back().SubMeshIndex == queued.SubMeshIndex)
						{
							s_RendererData.IndirectDrawData.back().InstancesCount++;
						}
						else
						{
							auto& command = s_RendererData.IndirectDrawData.emplace_back();
							command.InstancesCount = 1;
							command.SubMeshIndex = queued.SubMeshIndex;
						}
					}

					currentInstance++;
				}

				FlushShadowPassInstances(baseInstance);
			}

			commandBuffer->EndRenderTarget();
		}

		commandBuffer->StopTimer(s_RendererData.ShadowPassTimer);

		s_RendererData.CurrentInstancingMesh.Reset();
		s_RendererData.InstanceDataBuffer.clear();
	}

	void Renderer::FlushInstances(uint32_t instanceCount, uint32_t baseInstance)
	{
		FLARE_PROFILE_FUNCTION();

		if (instanceCount == 0 || s_RendererData.CurrentInstancingMesh.Mesh == nullptr)
			return;

		auto mesh = s_RendererData.CurrentInstancingMesh;
		const SubMesh& subMesh = mesh.Mesh->GetSubMeshes()[mesh.SubMeshIndex];

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();
		commandBuffer->DrawIndexed(mesh.Mesh, mesh.SubMeshIndex, baseInstance, instanceCount);

		s_RendererData.Statistics.DrawCallsCount++;
		s_RendererData.Statistics.DrawCallsSavedByInstancing += instanceCount - 1;
	}

	void Renderer::FlushShadowPassInstances(uint32_t baseInstance)
	{
		FLARE_PROFILE_FUNCTION();

		if (s_RendererData.CurrentInstancingMesh.Mesh == nullptr)
			return;

		uint32_t instancesCount = 0;
		for (auto& command : s_RendererData.IndirectDrawData)
			instancesCount += command.InstancesCount;

		if (instancesCount == 0)
			return;

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();
		for (const auto& drawCall : s_RendererData.IndirectDrawData)
		{
			commandBuffer->DrawIndexed(s_RendererData.CurrentInstancingMesh.Mesh, drawCall.SubMeshIndex, baseInstance, drawCall.InstancesCount);
			baseInstance += drawCall.InstancesCount;
		}

		s_RendererData.Statistics.DrawCallsCount++;
		s_RendererData.Statistics.DrawCallsSavedByInstancing += (uint32_t)instancesCount - 1;

		s_RendererData.IndirectDrawData.clear();
	}

	void Renderer::EndScene()
	{
		s_RendererData.Statistics.ShadowPassTime += s_RendererData.ShadowPassTimer->GetElapsedTime().value_or(0.0f);
		s_RendererData.Statistics.GeometryPassTime += s_RendererData.GeometryPassTimer->GetElapsedTime().value_or(0.0f);

		s_RendererData.PointLights.clear();
		s_RendererData.SpotLights.clear();
	}

	void Renderer::SubmitPointLight(const PointLightData& light)
	{
		s_RendererData.PointLights.push_back(light);
	}

	void Renderer::SubmitSpotLight(const SpotLightData& light)
	{
		s_RendererData.SpotLights.push_back(light);
	}

	void Renderer::DrawFullscreenQuad(const Ref<Material>& material)
	{
		FLARE_PROFILE_FUNCTION();

		if (!material || !material->GetShader())
			return;

		ApplyMaterial(material);

		RenderCommand::DrawIndexed(RendererPrimitives::GetFullscreenQuad());
		s_RendererData.Statistics.DrawCallsCount++;
	}

	void Renderer::DrawMesh(const Ref<VertexArray>& mesh, const Ref<Material>& material, size_t indicesCount)
	{
		FLARE_PROFILE_FUNCTION();

		ApplyMaterial(material);

		RenderCommand::DrawIndexed(mesh, indicesCount == SIZE_MAX ? mesh->GetIndexBuffer()->GetCount() : indicesCount);

		s_RendererData.Statistics.DrawCallsCount++;
	}

	void Renderer::DrawMesh(const Ref<Mesh>& mesh, uint32_t subMesh, const Ref<Material>& material, const glm::mat4& transform, MeshRenderFlags flags, int32_t entityIndex)
	{
		s_RendererData.OpaqueQueue.Submit(mesh, subMesh, material, transform, flags, entityIndex);
	}

	void Renderer::SubmitDecal(const Ref<const Material>& material, const glm::mat4& transform, int32_t entityIndex)
	{
		if (material == nullptr || material->GetShader() == nullptr)
			return;

		auto& decal = s_RendererData.Decals.emplace_back();
		decal.EntityIndex = entityIndex;
		decal.Material = material;
		decal.Transform = transform;
	}

	void Renderer::AddRenderPass(Ref<RenderPass> pass)
	{
		switch (pass->GetQueue())
		{
		case RenderPassQueue::BeforeShadows:
			s_RendererData.Passes.BeforeShadowsPasses.push_back(pass);
			break;
		case RenderPassQueue::BeforeOpaqueGeometry:
			s_RendererData.Passes.BeforeOpaqueGeometryPasses.push_back(pass);
			break;
		case RenderPassQueue::AfterOpaqueGeometry:
			s_RendererData.Passes.AfterOpaqueGeometryPasses.push_back(pass);
			break;
		case RenderPassQueue::PostProcessing:
			s_RendererData.Passes.PostProcessingPasses.push_back(pass);
			break;
		}
	}

	static void RemoveRenderPassFromList(std::vector<Ref<RenderPass>>& passes, const Ref<RenderPass>& pass)
	{
		for (size_t i = 0; i < passes.size(); i++)
		{
			if (passes[i] == pass)
			{
				passes.erase(passes.begin() + i);
				break;
			}
		}
	}

	void Renderer::RemoveRenderPass(Ref<RenderPass> pass)
	{
		if (pass == nullptr)
			return;

		switch (pass->GetQueue())
		{
		case RenderPassQueue::BeforeShadows:
			RemoveRenderPassFromList(s_RendererData.Passes.BeforeShadowsPasses, pass);
			break;
		case RenderPassQueue::BeforeOpaqueGeometry:
			RemoveRenderPassFromList(s_RendererData.Passes.BeforeOpaqueGeometryPasses, pass);
			break;
		case RenderPassQueue::AfterOpaqueGeometry:
			RemoveRenderPassFromList(s_RendererData.Passes.AfterOpaqueGeometryPasses, pass);
			break;
		case RenderPassQueue::PostProcessing:
			RemoveRenderPassFromList(s_RendererData.Passes.PostProcessingPasses, pass);
			break;
		}
	}

	void Renderer::ExecuteRenderPasses(std::vector<Ref<RenderPass>>& passes)
	{
		FLARE_CORE_ASSERT(s_RendererData.CurrentViewport);

		RenderingContext context(s_RendererData.CurrentViewport->RenderTarget, s_RendererData.CurrentViewport->RTPool);
		for (Ref<RenderPass>& pass : passes)
			pass->OnRender(context);
	}

	void Renderer::ExecutePostProcessingPasses()
	{
		if (!s_RendererData.CurrentViewport->PostProcessingEnabled)
			return;

		FLARE_PROFILE_FUNCTION();
		ExecuteRenderPasses(s_RendererData.Passes.PostProcessingPasses);
	}

	RendererSubmitionQueue& Renderer::GetOpaqueSubmitionQueue()
	{
		return s_RendererData.OpaqueQueue;
	}

	Viewport& Renderer::GetMainViewport()
	{
		FLARE_CORE_ASSERT(s_RendererData.MainViewport);
		return *s_RendererData.MainViewport;
	}

	Viewport& Renderer::GetCurrentViewport()
	{
		FLARE_CORE_ASSERT(s_RendererData.CurrentViewport);
		return *s_RendererData.CurrentViewport;
	}

	Ref<Texture> Renderer::GetWhiteTexture()
	{
		return s_RendererData.WhiteTexture;
	}

	Ref<Texture> Renderer::GetDefaultNormalMap()
	{
		return s_RendererData.DefaultNormalMap;
	}

	Ref<Material> Renderer::GetErrorMaterial()
	{
		return s_RendererData.ErrorMaterial;
	}

	Ref<FrameBuffer> Renderer::GetShadowsRenderTarget(size_t index)
	{
		return s_RendererData.ShadowsRenderTarget[index];
	}

	ShadowSettings& Renderer::GetShadowSettings()
	{
		return s_RendererData.ShadowMappingSettings;
	}

	Ref<VulkanDescriptorSet> Renderer::GetPrimaryDescriptorSet()
	{
		return s_RendererData.PrimaryDescriptorSet;
	}

	Ref<VulkanDescriptorSetLayout> Renderer::GetPrimaryDescriptorSetLayout()
	{
		return s_RendererData.PrimaryDescriptorPool->GetLayout();
	}
}
