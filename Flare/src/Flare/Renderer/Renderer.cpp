#include "Renderer.h"

#include "Flare/AssetManager/AssetManager.h"

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
	FLARE_IMPL_TYPE(ShadowSettings);

	struct InstanceData
	{
		glm::mat4 Transform;
		int32_t EntityIndex;
		int32_t Padding[3];
	};

	struct RenderableObject
	{
		Ref<Mesh> Mesh;
		Ref<Material> Material;
		uint32_t SubMeshIndex;
		glm::mat4 Transform;
		MeshRenderFlags Flags;
		int32_t EntityIndex;

		float SortKey;
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
	};

	struct InstancingMesh
	{
		inline void Reset()
		{
			Mesh = nullptr;
			SubMeshIndex = UINT32_MAX;
		}

		Ref<Mesh> Mesh = nullptr;
		uint32_t SubMeshIndex = UINT32_MAX;
	};

	struct RendererData
	{
		Viewport* MainViewport = nullptr;
		Viewport* CurrentViewport = nullptr;

		Ref<UniformBuffer> CameraBuffer = nullptr;
		Ref<UniformBuffer> LightBuffer = nullptr;
		Ref<VertexArray> FullscreenQuad = nullptr;

		Ref<Texture> WhiteTexture = nullptr;
		
		std::vector<Ref<RenderPass>> RenderPasses;
		RendererStatistics Statistics;

		std::vector<RenderableObject> Queue;
		std::vector<uint32_t> CulledObjectIndices;

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

	static void ReloadShaders()
	{
		std::optional<AssetHandle> errorShaderHandle = ShaderLibrary::FindShader("Error");
		if (errorShaderHandle && AssetManager::IsAssetHandleValid(*errorShaderHandle))
			s_RendererData.ErrorMaterial = CreateRef<Material>(AssetManager::GetAsset<Shader>(*errorShaderHandle));
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

		s_RendererData.ShadowMappingSettings.Resolution = ShadowSettings::ShadowResolution::_2048;
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

			for (size_t objectIndex = 0; objectIndex < s_RendererData.Queue.size(); objectIndex++)
			{
				const RenderableObject& object = s_RendererData.Queue[objectIndex];
				if (HAS_BIT(object.Flags, MeshRenderFlags::DontCastShadows))
					continue;

				Math::AABB objectAABB = object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds.Transformed(object.Transform);
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

				// 3. Extend near and far planes

				Math::Plane nearPlane = Math::Plane::TroughPoint(params.CameraFrustumCenter, -lightDirection);
				Math::Plane farPlane = Math::Plane::TroughPoint(params.CameraFrustumCenter, lightDirection);

				float nearPlaneDistance = 0;
				float farPlaneDistance = 0;

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
		shadowData.Resolution = (float)s_RendererData.ShadowMappingSettings.Resolution;

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

		{
			FLARE_PROFILE_SCOPE("UpdateLightUniformBuffer");
			s_RendererData.CurrentViewport->FrameData.Light.PointLightsCount = (uint32_t)s_RendererData.PointLights.size();
			s_RendererData.CurrentViewport->FrameData.Light.SpotLightsCount = (uint32_t)s_RendererData.SpotLights.size();
			s_RendererData.LightBuffer->SetData(&viewport.FrameData.Light, sizeof(viewport.FrameData.Light), 0);
		}

		{
			FLARE_PROFILE_SCOPE("UpdateCameraUniformBuffer");
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
			uint32_t size = (uint32_t)s_RendererData.ShadowMappingSettings.Resolution;
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

	static void ApplyMaterialFeatures(ShaderFeatures features)
	{
		RenderCommand::SetDepthTestEnabled(features.DepthTesting);
		RenderCommand::SetCullingMode(features.Culling);
		RenderCommand::SetDepthComparisonFunction(features.DepthFunction);
		RenderCommand::SetDepthWriteEnabled(features.DepthWrite);
		RenderCommand::SetBlendMode(features.Blending);
	}

	static bool CompareRenderableObjects(uint32_t aIndex, uint32_t bIndex)
	{
		const RenderableObject& a = s_RendererData.Queue[aIndex];
		const RenderableObject& b = s_RendererData.Queue[bIndex];

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
		for (size_t i = 0; i < s_RendererData.Queue.size(); i++)
		{
			const RenderableObject& object = s_RendererData.Queue[i];
			objectAABB = object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds.Transformed(object.Transform);

			bool intersects = true;
			for (size_t i = 0; i < planes.PlanesCount; i++)
			{
				intersects &= objectAABB.IntersectsOrInFrontOfPlane(planes.Planes[i]);
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

		s_RendererData.Statistics.ObjectsSubmitted += (uint32_t)s_RendererData.Queue.size();

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

			s_RendererData.Statistics.ObjectsCulled += (uint32_t)(s_RendererData.Queue.size() - s_RendererData.CulledObjectIndices.size());

			{
				FLARE_PROFILE_SCOPE("Sort");
				std::sort(s_RendererData.CulledObjectIndices.begin(), s_RendererData.CulledObjectIndices.end(), CompareRenderableObjects);
			}
		}

		FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();
		for (size_t i = 0; i < 4; i++)
			s_RendererData.ShadowsRenderTarget[i]->BindAttachmentTexture(0, 2 + (uint32_t)i);

		ExecuteGeomertyPass();
		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);

		s_RendererData.InstanceDataBuffer.clear();
		s_RendererData.CulledObjectIndices.clear();
		s_RendererData.Queue.clear();

		s_RendererData.GeometryPassTimer->Stop();
	}

	void Renderer::ExecuteGeomertyPass()
	{
		FLARE_PROFILE_FUNCTION();

		Ref<Material> currentMaterial = nullptr;

		for (uint32_t objectIndex : s_RendererData.CulledObjectIndices)
		{
			const RenderableObject& object = s_RendererData.Queue[objectIndex];
			if (s_RendererData.CurrentInstancingMesh.Mesh.get() != object.Mesh.get()
				|| s_RendererData.CurrentInstancingMesh.SubMeshIndex != object.SubMeshIndex)
			{
				FlushInstances();
				s_RendererData.CurrentInstancingMesh.Mesh = object.Mesh;
				s_RendererData.CurrentInstancingMesh.SubMeshIndex = object.SubMeshIndex;
			}

			if (object.Material.get() != currentMaterial.get())
			{
				FlushInstances();

				currentMaterial = object.Material;

				ApplyMaterialFeatures(object.Material->GetShader()->GetFeatures());

				currentMaterial->SetShaderProperties();

				Ref<Shader> shader = object.Material->GetShader();
				if (shader == nullptr)
					continue;

				FrameBufferAttachmentsMask shaderOutputsMask = 0;
				for (uint32_t output : shader->GetOutputs())
					shaderOutputsMask |= (1 << output);

				s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(shaderOutputsMask);
			}

			auto& instanceData = s_RendererData.InstanceDataBuffer.emplace_back();
			instanceData.Transform = object.Transform;
			instanceData.EntityIndex = object.EntityIndex;

			if (s_RendererData.InstanceDataBuffer.size() == (size_t)s_RendererData.MaxInstances)
				FlushInstances();
		}

		FlushInstances();
		s_RendererData.CurrentInstancingMesh.Reset();
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
				const auto& a = s_RendererData.Queue[aIndex];
				const auto& b = s_RendererData.Queue[bIndex];

				if (a.Mesh->Handle == b.Mesh->Handle)
					return a.SubMeshIndex < b.SubMeshIndex;

				return (uint64_t)a.Mesh->Handle < (uint64_t)b.Mesh->Handle;
			});

			// Fill instances buffer
			s_RendererData.InstanceDataBuffer.clear();
			for (uint32_t i : perCascadeObjects[cascadeIndex])
			{
				auto& instanceData = s_RendererData.InstanceDataBuffer.emplace_back();
				instanceData.Transform = s_RendererData.Queue[i].Transform;
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
				const auto& queued = s_RendererData.Queue[objectIndex];
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

	void Renderer::FlushInstances()
	{
		FLARE_PROFILE_FUNCTION();

		size_t instancesCount = s_RendererData.InstanceDataBuffer.size();
		if (instancesCount == 0 || s_RendererData.CurrentInstancingMesh.Mesh == nullptr)
			return;

		{
			FLARE_PROFILE_SCOPE("Renderer::FlushInstance::SetInstancesData");
			s_RendererData.InstancesShaderBuffer->SetData(MemorySpan::FromVector(s_RendererData.InstanceDataBuffer));
		}

		auto mesh = s_RendererData.CurrentInstancingMesh;

		const SubMesh& subMesh = mesh.Mesh->GetSubMeshes()[mesh.SubMeshIndex];
		RenderCommand::DrawInstancesIndexed(mesh.Mesh, mesh.SubMeshIndex, instancesCount);

		s_RendererData.Statistics.DrawCallsCount++;
		s_RendererData.Statistics.DrawCallsSavedByInstancing += (uint32_t)instancesCount - 1;

		s_RendererData.InstanceDataBuffer.clear();
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

		material->SetShaderProperties();
		ApplyMaterialFeatures(material->GetShader()->GetFeatures());

		const ShaderOutputs& shaderOutputs = material->GetShader()->GetOutputs();

		FrameBufferAttachmentsMask shaderOutputsMask = 0;
		for (uint32_t output : shaderOutputs)
			shaderOutputsMask |= (1 << output);

		FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();
		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(shaderOutputsMask);

		RenderCommand::DrawIndexed(s_RendererData.FullscreenQuad);
		s_RendererData.Statistics.DrawCallsCount++;

		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);
	}

	void Renderer::DrawMesh(const Ref<VertexArray>& mesh, const Ref<Material>& material, size_t indicesCount)
	{
		FLARE_PROFILE_FUNCTION();

		material->SetShaderProperties();
		ApplyMaterialFeatures(material->GetShader()->GetFeatures());

		const ShaderOutputs& shaderOutputs = material->GetShader()->GetOutputs();

		FrameBufferAttachmentsMask shaderOutputsMask = 0;
		for (uint32_t output : shaderOutputs)
			shaderOutputsMask |= (1 << output);

		FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();
		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(shaderOutputsMask);

		{
			FLARE_PROFILE_SCOPE("Renderer::DrawMesh::DrawIndexed");
			RenderCommand::DrawIndexed(mesh, indicesCount == SIZE_MAX ? mesh->GetIndexBuffer()->GetCount() : indicesCount);
		}

		s_RendererData.Statistics.DrawCallsCount++;

		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);
	}

	void Renderer::DrawMesh(const Ref<Mesh>& mesh, uint32_t subMesh, const Ref<Material>& material, const glm::mat4& transform, MeshRenderFlags flags, int32_t entityIndex)
	{
		FLARE_PROFILE_FUNCTION();
		if (s_RendererData.ErrorMaterial == nullptr)
			return;

		RenderableObject& object = s_RendererData.Queue.emplace_back();

		if (material)
			object.Material = material->GetShader() == nullptr ? s_RendererData.ErrorMaterial : material;
		else
			object.Material = s_RendererData.ErrorMaterial;

		glm::vec3 center = mesh->GetSubMeshes()[subMesh].Bounds.GetCenter();
		center = transform * glm::vec4(center, 1.0f);

		object.Flags = flags;
		object.Mesh = mesh;
		object.SubMeshIndex = subMesh;
		object.Transform = transform;
		object.EntityIndex = entityIndex;
		object.SortKey = glm::distance2(center, s_RendererData.CurrentViewport->FrameData.Camera.Position);
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
