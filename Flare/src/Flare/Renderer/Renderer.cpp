#include "Renderer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/Renderer.h"

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
	};

	struct RenderableObject
	{
		Ref<Mesh> Mesh;
		Ref<Material> Material;
		uint32_t SubMeshIndex;
		glm::mat4 Transform;
		MeshRenderFlags Flags;
		int32_t EntityIndex;
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

		Ref<VertexBuffer> InstanceBuffer = nullptr;
		std::vector<InstanceData> InstanceDataBuffer;
		uint32_t MaxInstances = 1024;

		InstancingMesh CurrentInstancingMesh;
		Ref<FrameBuffer> ShadowsRenderTarget[4] = { nullptr };

		Ref<Material> ErrorMaterial = nullptr;
		Ref<Material> DepthOnlyMeshMaterial = nullptr;

		ShadowSettings ShadowMappingSettings;
		Ref<UniformBuffer> ShadowDataBuffer = nullptr;
	};
	
	RendererData s_RendererData;

	static void ReloadShaders()
	{
		std::optional<AssetHandle> errorShaderHandle = ShaderLibrary::FindShader("Error");
		if (errorShaderHandle && AssetManager::IsAssetHandleValid(*errorShaderHandle))
			s_RendererData.ErrorMaterial = CreateRef<Material>(AssetManager::GetAsset<Shader>(*errorShaderHandle));
		else
			FLARE_CORE_ERROR("Renderer: Failed to find Error shader");

		std::optional<AssetHandle> depthOnlyMeshShaderHandle = ShaderLibrary::FindShader("Error");
		if (errorShaderHandle && AssetManager::IsAssetHandleValid(*errorShaderHandle))
			s_RendererData.DepthOnlyMeshMaterial= CreateRef<Material>(AssetManager::GetAsset<Shader>(*depthOnlyMeshShaderHandle));
		else
			FLARE_CORE_ERROR("Renderer: Failed to find MeshDepthOnly shader");
	}

	void Renderer::Initialize()
	{
		s_RendererData.CameraBuffer = UniformBuffer::Create(sizeof(CameraData), 0);
		s_RendererData.LightBuffer = UniformBuffer::Create(sizeof(LightData), 1);
		s_RendererData.ShadowDataBuffer = UniformBuffer::Create(sizeof(ShadowData), 2);

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

		uint32_t indices[] = {
			0, 1, 2,
			0, 2, 3,
		};

		Ref<VertexBuffer> vertexBuffer = VertexBuffer::Create(sizeof(vertices), (const void*)vertices);
		vertexBuffer->SetLayout({
			BufferLayoutElement("i_Position", ShaderDataType::Float2),
		});

		s_RendererData.FullscreenQuad = VertexArray::Create();
		s_RendererData.FullscreenQuad->SetIndexBuffer(IndexBuffer::Create(6, (const void*)indices));
		s_RendererData.FullscreenQuad->AddVertexBuffer(vertexBuffer);
		s_RendererData.FullscreenQuad->Unbind();

		{
			uint32_t whiteTextureData = 0xffffffff;
			s_RendererData.WhiteTexture = Texture::Create(1, 1, &whiteTextureData, TextureFormat::RGBA8);
		}

		s_RendererData.InstanceBuffer = VertexBuffer::Create(s_RendererData.MaxInstances * sizeof(InstanceData));
		s_RendererData.InstanceBuffer->SetLayout({
			{ "i_Transform", ShaderDataType::Matrix4x4 },
			{ "i_EntityIndex", ShaderDataType::Int },
		});

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

	static void CalculateShadowProjections()
	{
		FLARE_PROFILE_FUNCTION();

		Viewport* viewport = s_RendererData.CurrentViewport;
		glm::vec3 lightDirection = viewport->FrameData.Light.Direction;
		const Math::Basis& lightBasis = viewport->FrameData.LightBasis;
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();

		std::vector<uint32_t> perCascadeObjects[ShadowSettings::MaxCascades];

		{
			FLARE_PROFILE_SCOPE("DivideIntoGroups");

			ShadowMappingParams params;
			CalculateShadowFrustumParamsAroundCamera(params,
				lightDirection,
				*viewport,
				viewport->FrameData.Light.Near,
				shadowSettings.CascadeSplits[shadowSettings.Cascades - 1]);

			Math::Plane cameraNearPlane = Math::Plane::TroughPoint(
				s_RendererData.CurrentViewport->FrameData.Camera.Position,
				s_RendererData.CurrentViewport->FrameData.Camera.ViewDirection);
			
			for (size_t objectIndex = 0; objectIndex < s_RendererData.Queue.size(); objectIndex++)
			{
				const RenderableObject& object = s_RendererData.Queue[objectIndex];
				Math::AABB objectAABB = object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds.Transformed(object.Transform);

				float distanceToPlane = cameraNearPlane.SignedDistance(objectAABB.GetCenter());
				float projectedDistance = glm::dot(glm::abs(cameraNearPlane.Normal), objectAABB.GetExtents());

				size_t firstCascadeIndex = 0;
				size_t lastCascadeIndex = shadowSettings.Cascades - 1;

				for (size_t cascade = 0; cascade < shadowSettings.Cascades; cascade++)
				{
					if (distanceToPlane - projectedDistance <= shadowSettings.CascadeSplits[cascade])
					{
						firstCascadeIndex = cascade;
						break;
					}
				}

				for (size_t i = 0; i < shadowSettings.Cascades; i++)
				{
					if (distanceToPlane + projectedDistance <= shadowSettings.CascadeSplits[i])
					{
						lastCascadeIndex = i;
						break;
					}
				}

				if (firstCascadeIndex <= lastCascadeIndex)
				{
					for (size_t cascadeIndex = firstCascadeIndex; cascadeIndex <= lastCascadeIndex; cascadeIndex++)
						perCascadeObjects[cascadeIndex].push_back((uint32_t)objectIndex);
				}
			}
		}

		ShadowMappingParams perCascadeParams[ShadowSettings::MaxCascades];

		{
			// 1. Calculate a fit frustum around camera's furstum
			float currentNearPlane = viewport->FrameData.Light.Near;
			for (size_t i = 0; i < shadowSettings.Cascades; i++)
			{
				CalculateShadowFrustumParamsAroundCamera(perCascadeParams[i], lightDirection,
					*viewport, currentNearPlane,
					shadowSettings.CascadeSplits[i]);

				currentNearPlane = shadowSettings.CascadeSplits[i];
			}
		}

		float currentNearPlane = viewport->FrameData.Light.Near;
		for (size_t i = 0; i < shadowSettings.Cascades; i++)
		{
			ShadowMappingParams params = perCascadeParams[i];

			// 2. Calculate projection frustum planes (except near and far)

			Math::Plane planes[4];
			CalculateShadowProjectionFrustum(planes, params, lightDirection, lightBasis);

			// 3. Extend near and far planes

			Math::Plane nearPlane = Math::Plane::TroughPoint(params.CameraFrustumCenter, -lightDirection);
			Math::Plane farPlane = Math::Plane::TroughPoint(params.CameraFrustumCenter, lightDirection);

			float nearPlaneDistance = 0;
			float farPlaneDistance = 0;
			
			for (uint32_t objectIndex : perCascadeObjects[i])
			{
				const RenderableObject& object = s_RendererData.Queue[objectIndex];
				Math::AABB objectAABB = object.Mesh->GetSubMeshes()[object.SubMeshIndex].Bounds.Transformed(object.Transform);

				bool intersects = true;
				for (size_t i = 0; i < 4; i++)
				{
					if (!objectAABB.IntersectsOrInFrontOfPlane(planes[i]))
					{
						intersects = false;
						break;
					}
				}

				if (!intersects)
					continue;

				glm::vec3 center = objectAABB.GetCenter();
				glm::vec3 extents = objectAABB.Max - center;

				float projectedDistance = glm::dot(glm::abs(nearPlane.Normal), extents);
				nearPlaneDistance = glm::max(nearPlaneDistance, nearPlane.Distance(center) + projectedDistance);
				farPlaneDistance = glm::max(farPlaneDistance, farPlane.Distance(center) + projectedDistance);
			}

			nearPlaneDistance = -nearPlaneDistance;
			farPlaneDistance = farPlaneDistance;

			glm::mat4 view = glm::lookAt(params.CameraFrustumCenter + lightDirection * nearPlaneDistance, params.CameraFrustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

			CameraData& lightView = viewport->FrameData.LightView[i];
			lightView.View = view;
			lightView.Projection = glm::ortho(
				-params.BoundingSphereRadius,
				params.BoundingSphereRadius,
				-params.BoundingSphereRadius,
				params.BoundingSphereRadius,
				viewport->FrameData.Light.Near,
				farPlaneDistance - nearPlaneDistance);

			lightView.CalculateViewProjection();
			currentNearPlane = shadowSettings.CascadeSplits[i];
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
			FLARE_PROFILE_SCOPE("Renderer::UpdateLightUniformBuffer");
			s_RendererData.LightBuffer->SetData(&viewport.FrameData.Light, sizeof(viewport.FrameData.Light), 0);
		}

		{
			FLARE_PROFILE_SCOPE("Renderer::UpdateCameraUniformBuffer");
			s_RendererData.CameraBuffer->SetData(&viewport.FrameData.Camera, sizeof(CameraData), 0);
		}

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

		for (size_t i = 0; i < s_RendererData.ShadowMappingSettings.Cascades; i++)
		{
			s_RendererData.ShadowsRenderTarget[i]->Bind();
			RenderCommand::Clear();
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
	}

	static bool CompareRenderableObjects(uint32_t aIndex, uint32_t bIndex)
	{
		const RenderableObject& a = s_RendererData.Queue[aIndex];
		const RenderableObject& b = s_RendererData.Queue[bIndex];

		glm::vec3 aPosition = glm::vec3(a.Transform[3]);
		glm::vec3 bPosition = glm::vec3(b.Transform[3]);

		float aDistance = glm::distance2(aPosition, s_RendererData.CurrentViewport->FrameData.Camera.Position);
		float bDistance = glm::distance2(bPosition, s_RendererData.CurrentViewport->FrameData.Camera.Position);

		return aDistance < bDistance;

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
			FLARE_PROFILE_SCOPE("Renderer::GeometryPass");

			RenderCommand::SetViewport(0, 0, s_RendererData.CurrentViewport->GetSize().x, s_RendererData.CurrentViewport->GetSize().y);
			s_RendererData.CurrentViewport->RenderTarget->Bind();

			s_RendererData.CameraBuffer->SetData(
				&s_RendererData.CurrentViewport->FrameData.Camera,
				sizeof(s_RendererData.CurrentViewport->FrameData.Camera), 0);

			s_RendererData.CulledObjectIndices.clear();

			PerformFrustumCulling();

			s_RendererData.Statistics.ObjectsCulled += (uint32_t)(s_RendererData.Queue.size() - s_RendererData.CulledObjectIndices.size());

			std::sort(s_RendererData.CulledObjectIndices.begin(), s_RendererData.CulledObjectIndices.end(), CompareRenderableObjects);

			FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();
		
			for (size_t i = 0; i < 4; i++)
			{
				s_RendererData.ShadowsRenderTarget[i]->BindAttachmentTexture(0, 2 + (uint32_t)i);
			}

			ExecuteGeomertyPass();
			s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);

			s_RendererData.InstanceDataBuffer.clear();
			s_RendererData.CulledObjectIndices.clear();
			s_RendererData.Queue.clear();
		}
	}

	void Renderer::ExecuteGeomertyPass()
	{
		FLARE_PROFILE_FUNCTION();

		Ref<Material> currentMaterial = nullptr;

		for (uint32_t objectIndex : s_RendererData.CulledObjectIndices)
		{
			const RenderableObject& object = s_RendererData.Queue[objectIndex];
			if (object.Mesh->GetInstanceBuffer() == nullptr)
				object.Mesh->SetInstanceBuffer(s_RendererData.InstanceBuffer);

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

		CalculateShadowProjections();
		CalculateShadowMappingParams();

		// Prepare objects for shadow pass

		s_RendererData.CulledObjectIndices.clear();
		s_RendererData.CulledObjectIndices.reserve(s_RendererData.Queue.size());
		for (size_t i = 0; i < s_RendererData.Queue.size(); i++)
			s_RendererData.CulledObjectIndices.push_back((uint32_t)i);

		std::sort(s_RendererData.CulledObjectIndices.begin(), s_RendererData.CulledObjectIndices.end(), CompareRenderableObjects);

		// Setup the material

		RenderCommand::SetDepthTestEnabled(true);
		RenderCommand::SetCullingMode(CullingMode::Front);

		s_RendererData.DepthOnlyMeshMaterial->SetShaderProperties();

		for (size_t i = 0; i < s_RendererData.ShadowMappingSettings.Cascades; i++)
		{
			const FrameBufferSpecifications& shadowMapSpecs = s_RendererData.ShadowsRenderTarget[i]->GetSpecifications();
			RenderCommand::SetViewport(0, 0, shadowMapSpecs.Width, shadowMapSpecs.Height);

			s_RendererData.CameraBuffer->SetData(
				&s_RendererData.CurrentViewport->FrameData.LightView[i],
				sizeof(s_RendererData.CurrentViewport->FrameData.LightView[i]), 0);

			s_RendererData.ShadowsRenderTarget[i]->Bind();

			s_RendererData.CurrentInstancingMesh.Reset();
			for (uint32_t objectIndex : s_RendererData.CulledObjectIndices)
			{
				const auto& queued = s_RendererData.Queue[objectIndex];

				if (queued.Mesh->GetInstanceBuffer()  == nullptr)
					queued.Mesh->SetInstanceBuffer(s_RendererData.InstanceBuffer);

				if (queued.Mesh.get() != s_RendererData.CurrentInstancingMesh.Mesh.get()
					|| queued.SubMeshIndex != s_RendererData.CurrentInstancingMesh.SubMeshIndex)
				{
					FlushInstances();
					s_RendererData.CurrentInstancingMesh.Mesh = queued.Mesh;
					s_RendererData.CurrentInstancingMesh.SubMeshIndex = queued.SubMeshIndex;
				}

				auto& instance = s_RendererData.InstanceDataBuffer.emplace_back();
				instance.Transform = queued.Transform;

				if (s_RendererData.InstanceDataBuffer.size() == (size_t)s_RendererData.MaxInstances)
					FlushInstances();
			}

			FlushInstances();
		}

		s_RendererData.CurrentInstancingMesh.Reset();
	}

	void Renderer::FlushInstances()
	{
		FLARE_PROFILE_FUNCTION();

		size_t instancesCount = s_RendererData.InstanceDataBuffer.size();
		if (instancesCount == 0 || s_RendererData.CurrentInstancingMesh.Mesh == nullptr)
			return;

		{
			FLARE_PROFILE_SCOPE("Renderer::FlushInstance::SetInstancesData");
			s_RendererData.InstanceBuffer->SetData(s_RendererData.InstanceDataBuffer.data(), sizeof(InstanceData) * instancesCount);
		}

		auto mesh = s_RendererData.CurrentInstancingMesh;

		const SubMesh& subMesh = mesh.Mesh->GetSubMeshes()[mesh.SubMeshIndex];
		RenderCommand::DrawInstanced(mesh.Mesh->GetSubMeshes()[mesh.SubMeshIndex].VertexArray, instancesCount);

		s_RendererData.Statistics.DrawCallsCount++;
		s_RendererData.Statistics.DrawCallsSavedByInstancing += (uint32_t)instancesCount - 1;

		s_RendererData.InstanceDataBuffer.clear();
	}

	void Renderer::EndScene()
	{
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
		if (s_RendererData.ErrorMaterial == nullptr)
			return;

		RenderableObject& object = s_RendererData.Queue.emplace_back();

		if (material)
			object.Material = material->GetShader() == nullptr ? s_RendererData.ErrorMaterial : material;
		else
			object.Material = s_RendererData.ErrorMaterial;

		object.Flags = flags;
		object.Mesh = mesh;
		object.SubMeshIndex = subMesh;
		object.Transform = transform;
		object.EntityIndex = entityIndex;
	}

	void Renderer::AddRenderPass(Ref<RenderPass> pass)
	{
		s_RendererData.RenderPasses.push_back(pass);
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
