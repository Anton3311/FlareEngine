#include "Renderer.h"

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

#include <random>

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
		Ref<Material> Material;
	};

	struct RendererData
	{
		Viewport* MainViewport = nullptr;
		Viewport* CurrentViewport = nullptr;

		Ref<UniformBuffer> CameraBuffer = nullptr;
		Ref<UniformBuffer> LightBuffer = nullptr;
		Ref<VertexArray> FullscreenQuad = nullptr;

		Ref<Texture> WhiteTexture = nullptr;
		Ref<Mesh> CubeMesh = nullptr;
		
		std::vector<Ref<RenderPass>> RenderPasses;
		RendererStatistics Statistics;

		RendererSubmitionQueue OpaqueQueue;
		std::vector<uint32_t> CulledObjectIndices;
		std::vector<DecalData> Decals;

		std::vector<InstanceData> InstanceDataBuffer;
		uint32_t MaxInstances = 1024;

		InstancingMesh CurrentInstancingMesh;
		Ref<FrameBuffer> ShadowsRenderTarget[4] = { nullptr };

		Ref<Material> ErrorMaterial = nullptr;
		Ref<Material> DepthOnlyMeshMaterial = nullptr;

		ShadowSettings ShadowMappingSettings;
		Ref<UniformBuffer> ShadowDataBuffer = nullptr;

		Ref<ShaderStorageBuffer> InstancesShaderBuffer = nullptr;
		std::vector<DrawIndirectCommandSubMeshData> IndirectDrawData;

		std::vector<PointLightData> PointLights;
		Ref<ShaderStorageBuffer> PointLightsShaderBuffer = nullptr;
		std::vector<SpotLightData> SpotLights;
		Ref<ShaderStorageBuffer> SpotLightsShaderBuffer = nullptr;

		Ref<GPUTimer> ShadowPassTimer = nullptr;
		Ref<GPUTimer> GeometryPassTimer = nullptr;
	};
	
	RendererData s_RendererData;

	void Renderer::ReloadShaders()
	{
		std::optional<AssetHandle> errorShaderHandle = ShaderLibrary::FindShader("Error");
		if (errorShaderHandle && AssetManager::IsAssetHandleValid(*errorShaderHandle))
		{
			s_RendererData.ErrorMaterial = CreateRef<Material>(AssetManager::GetAsset<Shader>(*errorShaderHandle));
			s_RendererData.OpaqueQueue.m_ErrorMaterial = s_RendererData.ErrorMaterial;
		}
		else
			FLARE_CORE_ERROR("Renderer: Failed to find Error shader");

		std::optional<AssetHandle> depthOnlyMeshShaderHandle = ShaderLibrary::FindShader("MeshDepthOnly");
		if (depthOnlyMeshShaderHandle && AssetManager::IsAssetHandleValid(*depthOnlyMeshShaderHandle))
			s_RendererData.DepthOnlyMeshMaterial= CreateRef<Material>(AssetManager::GetAsset<Shader>(*depthOnlyMeshShaderHandle));
		else
			FLARE_CORE_ERROR("Renderer: Failed to find MeshDepthOnly shader");
	}

	void Renderer::Initialize()
	{
		s_RendererData.CameraBuffer = UniformBuffer::Create(sizeof(CameraData), 0);
		s_RendererData.LightBuffer = UniformBuffer::Create(sizeof(LightData), 1);
		s_RendererData.ShadowDataBuffer = UniformBuffer::Create(sizeof(ShadowData), 2);
		s_RendererData.InstancesShaderBuffer = ShaderStorageBuffer::Create(3);
		s_RendererData.PointLightsShaderBuffer = ShaderStorageBuffer::Create(4);
		s_RendererData.SpotLightsShaderBuffer = ShaderStorageBuffer::Create(5);

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

		float vertices[] = {
			-1, -1,
			-1,  1,
			 1,  1,
			 1, -1,
		};

		uint16_t indices[] = {
			0, 1, 2,
			0, 2, 3,
		};

		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(sizeof(vertices), (const void*)vertices);
		vertexBuffer->SetLayout({
			BufferLayoutElement("i_Position", ShaderDataType::Float2),
		});

		s_RendererData.FullscreenQuad = VertexArray::Create();
		s_RendererData.FullscreenQuad->SetIndexBuffer(IndexBuffer::Create(IndexBuffer::IndexFormat::UInt16, MemorySpan(indices, 6)));
		s_RendererData.FullscreenQuad->AddVertexBuffer(vertexBuffer);
		s_RendererData.FullscreenQuad->Unbind();

		{
			uint32_t whiteTextureData = 0xffffffff;
			s_RendererData.WhiteTexture = Texture::Create(1, 1, &whiteTextureData, TextureFormat::RGBA8);
		}

		{
			glm::vec3 cubeVertices[] =
			{
				glm::vec3(-0.5f, -0.5f, -0.5f),
				glm::vec3(+0.5f, -0.5f, -0.5f),
				glm::vec3(+0.5f, +0.5f, -0.5f),
				glm::vec3(-0.5f, +0.5f, -0.5f),

				glm::vec3(-0.5f, -0.5f, +0.5f),
				glm::vec3(+0.5f, -0.5f, +0.5f),
				glm::vec3(+0.5f, +0.5f, +0.5f),
				glm::vec3(-0.5f, +0.5f, +0.5f),
			};

			glm::vec3 cubeNormals[] =
			{
				glm::vec3(0.0f),
				glm::vec3(0.0f),
				glm::vec3(0.0f),
				glm::vec3(0.0f),

				glm::vec3(0.0f),
				glm::vec3(0.0f),
				glm::vec3(0.0f),
				glm::vec3(0.0f),
			};

			glm::vec2 cubeUVs[] =
			{
				glm::vec2(0.0f),
				glm::vec2(0.0f),
				glm::vec2(0.0f),
				glm::vec2(0.0f),

				glm::vec2(0.0f),
				glm::vec2(0.0f),
				glm::vec2(0.0f),
				glm::vec2(0.0f),
			};

			uint16_t cubeIndices[] =
			{
				// Front
				1, 2, 0,
				2, 3, 0,

				// Left
				3, 4, 0,
				4, 3, 7,

				// Right
				6, 2, 1,
				6, 1, 5,

				// Back
				6, 5, 4,
				7, 6, 4,

				// Bottom
				0, 4, 1,
				1, 4, 5,

				// Top
				7, 3, 2,
				6, 7, 2
			};

			s_RendererData.CubeMesh = CreateRef<Mesh>(MeshTopology::Triangles, 8, IndexBuffer::IndexFormat::UInt16, sizeof(cubeIndices) / 2);
			s_RendererData.CubeMesh->AddSubMesh(
				Span(cubeVertices, 8),
				MemorySpan(cubeIndices, sizeof(cubeIndices) / 2),
				Span(cubeNormals, 8),
				Span(cubeUVs, 8));
		}

		Project::OnProjectOpen.Bind(ReloadShaders);
	}

	void Renderer::Shutdown()
	{
		s_RendererData.FullscreenQuad = nullptr;
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
		const CameraData& camera = viewport.FrameData.Camera;

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

	static void CalculateShadowProjectionFrustum(Math::Plane* outPlanes, const ShadowMappingParams& params, glm::vec3 lightDirection, const Math::Basis& lightBasis)
	{
		// Near and far planes aren't calculated here
		// because they are computed based on AABBs of scene objects

		// Left
		outPlanes[0] = Math::Plane::TroughPoint(
			params.CameraFrustumCenter - lightBasis.Right * params.BoundingSphereRadius,
			lightBasis.Right);

		 // Right
		outPlanes[1] = Math::Plane::TroughPoint(
			params.CameraFrustumCenter + lightBasis.Right * params.BoundingSphereRadius,
			-lightBasis.Right);

		// Top
		outPlanes[2] = Math::Plane::TroughPoint(
			params.CameraFrustumCenter + lightBasis.Up * params.BoundingSphereRadius,
			-lightBasis.Up);

		// Bottom
		outPlanes[3] = Math::Plane::TroughPoint(
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
		Math::Plane cascadePlanes[ShadowSettings::MaxCascades][4];

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
					cascadePlanes[i],
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
						if (!objectAABB.IntersectsOrInFrontOfPlane(cascadePlanes[cascadeIndex][i]))
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

				glm::mat4 view = glm::lookAt(params.CameraFrustumCenter + lightDirection * nearPlaneDistance, params.CameraFrustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

				CameraData& lightView = viewport->FrameData.LightView[cascadeIndex];
				lightView.View = view;
				lightView.Projection = glm::ortho(
					-params.BoundingSphereRadius,
					params.BoundingSphereRadius,
					-params.BoundingSphereRadius,
					params.BoundingSphereRadius,
					viewport->FrameData.Light.Near,
					farPlaneDistance - nearPlaneDistance);

				lightView.CalculateViewProjection();
				currentNearPlane = shadowSettings.CascadeSplits[cascadeIndex];
			}
		}
	}

	static void CalculateShadowMappingParams()
	{
		FLARE_PROFILE_FUNCTION();

		ShadowData shadowData;
		shadowData.Bias = s_RendererData.ShadowMappingSettings.Bias;
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

		s_RendererData.ShadowDataBuffer->SetData(&shadowData, sizeof(shadowData), 0);
	}

	void Renderer::BeginScene(Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		s_RendererData.CurrentViewport = &viewport;
		s_RendererData.OpaqueQueue.m_CameraPosition = viewport.FrameData.Camera.Position;

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
			s_RendererData.CameraBuffer->SetData(&viewport.FrameData.Camera, sizeof(CameraData), 0);
		}

		{
			FLARE_PROFILE_SCOPE("UploadPointLightsData");
			s_RendererData.PointLightsShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.PointLights));
		}

		{
			FLARE_PROFILE_SCOPE("UploadSpotLightsData");
			s_RendererData.SpotLightsShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.SpotLights));
		}

		{
			FLARE_PROFILE_SCOPE("ResizeShadowBuffers");
			uint32_t size = (uint32_t)GetShadowMapResolution(s_RendererData.ShadowMappingSettings.Quality);
			if (s_RendererData.ShadowsRenderTarget[0] == nullptr)
			{
				FrameBufferSpecifications shadowMapSpecs;
				shadowMapSpecs.Width = size;
				shadowMapSpecs.Height = size;
				shadowMapSpecs.Attachments = { { FrameBufferTextureFormat::Depth, TextureWrap::Clamp, TextureFiltering::Linear } };

				for (size_t i = 0; i < 4; i++)
					s_RendererData.ShadowsRenderTarget[i] = FrameBuffer::Create(shadowMapSpecs);
			}
			else
			{
				auto& shadowMapSpecs = s_RendererData.ShadowsRenderTarget[0]->GetSpecifications();
				if (shadowMapSpecs.Width != size || shadowMapSpecs.Height != size)
				{
					for (size_t i = 0; i < 4; i++)
						s_RendererData.ShadowsRenderTarget[i]->Resize(size, size);
				}
			}
		}

		{
			FLARE_PROFILE_SCOPE("ClearShadowBuffers");
			for (size_t i = 0; i < s_RendererData.ShadowMappingSettings.Cascades; i++)
			{
				s_RendererData.ShadowsRenderTarget[i]->Bind();
				RenderCommand::Clear();
			}
		}

		viewport.RenderTarget->Bind();

		// Generate camera frustum planes
		{
			FLARE_PROFILE_SCOPE("Renderer::GenerateFrustumPlanes");
			std::array<glm::vec4, 8> frustumCorners =
			{
				// Near
				glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),
				glm::vec4(1.0f, -1.0f, 0.0f, 1.0f),
				glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f),
				glm::vec4(1.0f,  1.0f, 0.0f, 1.0f),

				// Far
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

			FrustumPlanes& frustumPlanes = viewport.FrameData.CameraFrustumPlanes;

			// Near
			frustumPlanes.Planes[FrustumPlanes::NearPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[0], s_RendererData.CurrentViewport->FrameData.Camera.ViewDirection);

			// Far
			frustumPlanes.Planes[FrustumPlanes::FarPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[4], -s_RendererData.CurrentViewport->FrameData.Camera.ViewDirection);

			// Left (Trough bottom left corner)
			frustumPlanes.Planes[FrustumPlanes::LeftPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[0], glm::normalize(glm::cross(
				// Bottom Left Near -> Bottom Left Far
				(glm::vec3)(frustumCorners[4] - frustumCorners[0]),
				// Bottom Left Near -> Top Left Near
				(glm::vec3)(frustumCorners[2] - frustumCorners[0]))));

			// Right (Trough top right corner)
			frustumPlanes.Planes[FrustumPlanes::RightPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[1], glm::cross(
				// Top Right Near -> Top Right Far
				(glm::vec3)(frustumCorners[7] - frustumCorners[3]),
				// Top Right Near -> Bottom Right Near
				(glm::vec3)(frustumCorners[1] - frustumCorners[3])));

			// Top (Trough top right corner)
			frustumPlanes.Planes[FrustumPlanes::TopPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[3], glm::cross(
				// Top Right Near -> Top Left Near
				(glm::vec3)(frustumCorners[2] - frustumCorners[3]),
				// Top Right Near -> Top Right Far
				(glm::vec3)(frustumCorners[7] - frustumCorners[3])));

			// Bottom (Trough bottom left corner)
			frustumPlanes.Planes[FrustumPlanes::BottomPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[0], glm::cross(
				// Bottom left near => Bottom right near
				(glm::vec3)(frustumCorners[1] - frustumCorners[0]),
				// Bottom left near -> Bottom left far
				(glm::vec3)(frustumCorners[4] - frustumCorners[0])));
		}
	}

	static void ApplyMaterial(const Ref<const Material>& materail)
	{
		Ref<Shader> shader = materail->GetShader();
		FLARE_CORE_ASSERT(shader);

		ShaderFeatures features = materail->GetShader()->GetFeatures();

		RenderCommand::SetDepthTestEnabled(features.DepthTesting);
		RenderCommand::SetCullingMode(features.Culling);
		RenderCommand::SetDepthComparisonFunction(features.DepthFunction);
		RenderCommand::SetDepthWriteEnabled(features.DepthWrite);
		RenderCommand::SetBlendMode(features.Blending);

		FrameBufferAttachmentsMask shaderOutputsMask = 0;
		for (uint32_t output : shader->GetOutputs())
			shaderOutputsMask |= (1 << output);

		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(shaderOutputsMask);

		materail->SetShaderProperties();
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
			ExecuteShadowPass();

		// Geometry

		{
			FLARE_PROFILE_SCOPE("Renderer::PrepareGeometryPass");
			s_RendererData.GeometryPassTimer->Start();

			RenderCommand::SetViewport(0, 0, s_RendererData.CurrentViewport->GetSize().x, s_RendererData.CurrentViewport->GetSize().y);
			s_RendererData.CurrentViewport->RenderTarget->Bind();

			s_RendererData.CameraBuffer->SetData(
				&s_RendererData.CurrentViewport->FrameData.Camera,
				sizeof(s_RendererData.CurrentViewport->FrameData.Camera), 0);

			s_RendererData.CulledObjectIndices.clear();

			PerformFrustumCulling();

			s_RendererData.Statistics.ObjectsCulled += (uint32_t)(s_RendererData.OpaqueQueue.GetSize() - s_RendererData.CulledObjectIndices.size());

			{
				FLARE_PROFILE_SCOPE("Sort");
				std::sort(s_RendererData.CulledObjectIndices.begin(), s_RendererData.CulledObjectIndices.end(), CompareRenderableObjects);
			}
		}

		FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();

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

		ExecuteGeomertyPass();
		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);

		s_RendererData.InstanceDataBuffer.clear();
		s_RendererData.CulledObjectIndices.clear();
		s_RendererData.OpaqueQueue.m_Buffer.clear();

		s_RendererData.GeometryPassTimer->Stop();
	}

	void Renderer::ExecuteGeomertyPass()
	{
		FLARE_PROFILE_FUNCTION();

		Ref<const Material> currentMaterial = nullptr;

		s_RendererData.InstanceDataBuffer.clear();
		for (uint32_t objectIndex : s_RendererData.CulledObjectIndices)
		{
			auto& instanceData = s_RendererData.InstanceDataBuffer.emplace_back();
			instanceData.Transform = s_RendererData.OpaqueQueue[objectIndex].Transform.ToMatrix4x4();
			instanceData.EntityIndex = s_RendererData.OpaqueQueue[objectIndex].EntityIndex;
		}

		{
			FLARE_PROFILE_SCOPE("SetIntancesData");
			s_RendererData.InstancesShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.InstanceDataBuffer));
		}

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

		// Decals

		s_RendererData.InstanceDataBuffer.clear();

		std::sort(s_RendererData.Decals.begin(), s_RendererData.Decals.end(), [](const DecalData& a, const DecalData& b) -> bool
		{
			return (uint64_t)a.Material->Handle < (uint64_t)b.Material->Handle;
		});

		for (const DecalData& decal : s_RendererData.Decals)
		{
			auto& instance = s_RendererData.InstanceDataBuffer.emplace_back();
			instance.Transform = decal.Transform;
			instance.EntityIndex = decal.EntityIndex;
		}

		s_RendererData.InstancesShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.InstanceDataBuffer));

		if (s_RendererData.Decals.size() > 0)
		{
			s_RendererData.CurrentViewport->RenderTarget->BindAttachmentTexture(s_RendererData.CurrentViewport->DepthAttachmentIndex, 1);

			uint32_t baseInstance = 0;
			Ref<Material> currentMaterial = s_RendererData.Decals[0].Material;

			uint32_t instanceIndex = 0;
			for (; instanceIndex < (uint32_t)s_RendererData.Decals.size(); instanceIndex++)
			{
				if (s_RendererData.Decals[instanceIndex].Material.get() != currentMaterial.get())
				{
					ApplyMaterial(currentMaterial);

					RenderCommand::DrawInstancesIndexed(s_RendererData.CubeMesh, 0, instanceIndex - baseInstance, baseInstance);
					baseInstance = instanceIndex;

					currentMaterial = s_RendererData.Decals[instanceIndex].Material;
				}
			}

			if (instanceIndex != baseInstance)
			{
				FLARE_CORE_ASSERT(currentMaterial->GetShader());
				ApplyMaterial(currentMaterial);

				RenderCommand::DrawInstancesIndexed(s_RendererData.CubeMesh, 0, instanceIndex - baseInstance, baseInstance);
			}
		}

		s_RendererData.Decals.clear();
		s_RendererData.InstanceDataBuffer.clear();
	}

	void Renderer::ExecuteShadowPass()
	{
		FLARE_PROFILE_FUNCTION();

		s_RendererData.ShadowPassTimer->Start();

		std::vector<uint32_t> perCascadeObjects[ShadowSettings::MaxCascades];

		CalculateShadowProjections(perCascadeObjects);
		CalculateShadowMappingParams();

		// Setup the material

		RenderCommand::SetDepthTestEnabled(true);
		RenderCommand::SetCullingMode(CullingMode::Front);

		s_RendererData.DepthOnlyMeshMaterial->SetShaderProperties();

		for (size_t cascadeIndex = 0; cascadeIndex < s_RendererData.ShadowMappingSettings.Cascades; cascadeIndex++)
		{
			const FrameBufferSpecifications& shadowMapSpecs = s_RendererData.ShadowsRenderTarget[cascadeIndex]->GetSpecifications();
			RenderCommand::SetViewport(0, 0, shadowMapSpecs.Width, shadowMapSpecs.Height);

			s_RendererData.CameraBuffer->SetData(
				&s_RendererData.CurrentViewport->FrameData.LightView[cascadeIndex],
				sizeof(s_RendererData.CurrentViewport->FrameData.LightView[cascadeIndex]), 0);

			s_RendererData.ShadowsRenderTarget[cascadeIndex]->Bind();
			s_RendererData.CurrentInstancingMesh.Reset();

			// Group objects with same meshes together
			std::sort(perCascadeObjects[cascadeIndex].begin(), perCascadeObjects[cascadeIndex].end(), [](uint32_t aIndex, uint32_t bIndex) -> bool
			{
				const auto& a = s_RendererData.OpaqueQueue[aIndex];
				const auto& b = s_RendererData.OpaqueQueue[bIndex];

				if (a.Mesh->Handle == b.Mesh->Handle)
					return a.SubMeshIndex < b.SubMeshIndex;

				return (uint64_t)a.Mesh->Handle < (uint64_t)b.Mesh->Handle;
			});

			// Fill instances buffer
			s_RendererData.InstanceDataBuffer.clear();
			for (uint32_t i : perCascadeObjects[cascadeIndex])
			{
				auto& instanceData = s_RendererData.InstanceDataBuffer.emplace_back();
				instanceData.Transform = s_RendererData.OpaqueQueue[i].Transform.ToMatrix4x4();
			}

			{
				FLARE_PROFILE_SCOPE("SetInstancesData");
				s_RendererData.InstancesShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.InstanceDataBuffer));
			}

			s_RendererData.IndirectDrawData.clear();
			s_RendererData.CurrentInstancingMesh.Reset();

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

		s_RendererData.CurrentInstancingMesh.Reset();
		s_RendererData.ShadowPassTimer->Stop();
		s_RendererData.InstanceDataBuffer.clear();
	}

	void Renderer::FlushInstances(uint32_t count, uint32_t baseInstance)
	{
		FLARE_PROFILE_FUNCTION();

		if (count == 0 || s_RendererData.CurrentInstancingMesh.Mesh == nullptr)
			return;

		auto mesh = s_RendererData.CurrentInstancingMesh;

		const SubMesh& subMesh = mesh.Mesh->GetSubMeshes()[mesh.SubMeshIndex];
		RenderCommand::DrawInstancesIndexed(mesh.Mesh, mesh.SubMeshIndex, count, baseInstance);

		s_RendererData.Statistics.DrawCallsCount++;
		s_RendererData.Statistics.DrawCallsSavedByInstancing += count - 1;
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

		RenderCommand::DrawInstancesIndexedIndirect(
			s_RendererData.CurrentInstancingMesh.Mesh,
			Span<DrawIndirectCommandSubMeshData>::FromVector(s_RendererData.IndirectDrawData),
			baseInstance);

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

		RenderCommand::DrawIndexed(s_RendererData.FullscreenQuad);
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

	void Renderer::SubmitDecal(const Ref<Material>& material, const glm::mat4& transform, int32_t entityIndex)
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
		s_RendererData.RenderPasses.push_back(pass);
	}

	void Renderer::RemoveRenderPass(Ref<RenderPass> pass)
	{
		for (size_t i = 0; i < s_RendererData.RenderPasses.size(); i++)
		{
			if (s_RendererData.RenderPasses[i] == pass)
			{
				s_RendererData.RenderPasses.erase(s_RendererData.RenderPasses.begin() + i);
				break;
			}
		}
	}

	void Renderer::ExecuteRenderPasses()
	{
		FLARE_CORE_ASSERT(s_RendererData.CurrentViewport);

		FLARE_PROFILE_FUNCTION();
		RenderingContext context(s_RendererData.CurrentViewport->RenderTarget, s_RendererData.CurrentViewport->RTPool);

		for (Ref<RenderPass>& pass : s_RendererData.RenderPasses)
			pass->OnRender(context);
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

	Ref<Mesh> Renderer::GetCubeMesh()
	{
		return s_RendererData.CubeMesh;
	}

	Ref<Material> Renderer::GetErrorMaterial()
	{
		return s_RendererData.ErrorMaterial;
	}

	Ref<const VertexArray> Renderer::GetFullscreenQuad()
	{
		return s_RendererData.FullscreenQuad;
	}

	Ref<FrameBuffer> Renderer::GetShadowsRenderTarget(size_t index)
	{
		return s_RendererData.ShadowsRenderTarget[index];
	}

	ShadowSettings& Renderer::GetShadowSettings()
	{
		return s_RendererData.ShadowMappingSettings;
	}
}
