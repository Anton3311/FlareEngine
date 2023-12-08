#include "Renderer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/Project/Project.h"

#include <random>

namespace Flare
{
	struct InstanceData
	{
		glm::mat4 Transform;
		int32_t EntityIndex;
	};

	struct RenderableObject
	{
		Ref<Mesh> Mesh;
		Ref<Material> Material;
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

		Ref<Mesh> CurrentInstancingMesh = nullptr;
		Ref<FrameBuffer> ShadowsRenderTarget[4] = { nullptr };

		Ref<Material> ErrorMaterial = nullptr;

		ShadowSettings ShadowMappingSettings;
		Ref<UniformBuffer> ShadowDataBuffer = nullptr;

		Ref<Texture3D> RandomAngles = nullptr;
	};
	
	RendererData s_RendererData;

	static void ReloadShaders()
	{
		std::optional<AssetHandle> errorShaderHandle = ShaderLibrary::FindShader("Error");
		if (errorShaderHandle && AssetManager::IsAssetHandleValid(*errorShaderHandle))
			s_RendererData.ErrorMaterial = CreateRef<Material>(AssetManager::GetAsset<Shader>(*errorShaderHandle));
		else
			FLARE_CORE_ERROR("Renderer: Failed to find Error shader");
	}

	void Renderer::Initialize()
	{
		s_RendererData.CameraBuffer = UniformBuffer::Create(sizeof(CameraData), 0);
		s_RendererData.LightBuffer = UniformBuffer::Create(sizeof(LightData), 1);
		s_RendererData.ShadowDataBuffer = UniformBuffer::Create(sizeof(ShadowData), 2);

		s_RendererData.ShadowMappingSettings.Resolution = 2048;
		s_RendererData.ShadowMappingSettings.Bias = 0.015f;
		s_RendererData.ShadowMappingSettings.LightSize = 0.05f;

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

		// Random angles

		{
			constexpr size_t anglesTextureSize = 16;
			float* angles = new float[anglesTextureSize * anglesTextureSize * anglesTextureSize];

			std::random_device device;
			std::mt19937_64 engine(device());
			std::uniform_real_distribution<float> uniformDistribution(0.0f, glm::pi<float>() * 2.0f);

			for (size_t z = 0; z < anglesTextureSize; z++)
			{
				for (size_t y = 0; y < anglesTextureSize; y++)
				{
					for (size_t x = 0; x < anglesTextureSize; x++)
					{
						angles[z * anglesTextureSize * anglesTextureSize + y * anglesTextureSize + x] = uniformDistribution(engine);
					}
				}
			}

			Texture3DSpecifications specifications;
			specifications.Filtering = TextureFiltering::Linear;
			specifications.Format = TextureFormat::RF32;
			specifications.Size = glm::uvec3((uint32_t)anglesTextureSize);
			s_RendererData.RandomAngles = Texture3D::Create(specifications, (const void*)angles, specifications.Size);

			delete[] angles;
		}
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
		s_RendererData.Statistics.DrawCallsSavedByInstances = 0;
	}

	void Renderer::SetMainViewport(Viewport& viewport)
	{
		s_RendererData.MainViewport = &viewport;
	}

	struct ShadowMappingParams
	{
		glm::vec3 ViewPosition;
		glm::vec3 CameraFrustumCenter;
		float BoundingSphereRadius;
	};

	static void CalculateShadowFrustums(ShadowMappingParams& params, glm::vec3 lightDirection, const Viewport& viewport, float nearPlaneDistance, float farPlaneDistance)
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

		params.ViewPosition = frustumCenter - lightDirection * boundingSphereRadius;
		params.CameraFrustumCenter = frustumCenter;
		params.BoundingSphereRadius = boundingSphereRadius;
	}

	static void CalculateShadowProjections()
	{
		Viewport* viewport = s_RendererData.CurrentViewport;

		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();
		float currentNearPlane = viewport->FrameData.Light.Near;
		for (size_t i = 0; i < 4; i++)
		{
			ShadowMappingParams params;
			CalculateShadowFrustums(params,
				viewport->FrameData.Light.Direction,
				*viewport, currentNearPlane,
				shadowSettings.CascadeSplits[i]);

			glm::mat4 view = glm::lookAt(params.ViewPosition, params.CameraFrustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

			CameraData& lightView = viewport->FrameData.LightView[i];
			lightView.View = view;
			lightView.Projection = glm::ortho(
				-params.BoundingSphereRadius,
				params.BoundingSphereRadius,
				-params.BoundingSphereRadius,
				params.BoundingSphereRadius,
				currentNearPlane, params.BoundingSphereRadius * 2.0f);

			lightView.CalculateViewProjection();
			currentNearPlane = shadowSettings.CascadeSplits[i];
		}
	}

	static void CalculateShadowMappingParams()
	{
		ShadowData shadowData;
		shadowData.Bias = s_RendererData.ShadowMappingSettings.Bias;
		shadowData.LightSize = s_RendererData.ShadowMappingSettings.LightSize;

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
		s_RendererData.CurrentViewport = &viewport;

		CalculateShadowProjections();
		CalculateShadowMappingParams();

		s_RendererData.LightBuffer->SetData(&viewport.FrameData.Light, sizeof(viewport.FrameData.Light), 0);
		s_RendererData.CameraBuffer->SetData(&viewport.FrameData.Camera, sizeof(CameraData), 0);

		if (s_RendererData.ShadowsRenderTarget[0] == nullptr)
		{
			FrameBufferSpecifications shadowMapSpecs;
			shadowMapSpecs.Width = s_RendererData.ShadowMappingSettings.Resolution;
			shadowMapSpecs.Height = s_RendererData.ShadowMappingSettings.Resolution;
			shadowMapSpecs.Attachments = { { FrameBufferTextureFormat::Depth, TextureWrap::Clamp, TextureFiltering::Linear } };

			for (size_t i = 0; i < 2; i++)
			{
				s_RendererData.ShadowsRenderTarget[i] = FrameBuffer::Create(shadowMapSpecs);
			}

			shadowMapSpecs.Width /= 2;
			shadowMapSpecs.Height /= 2;

			for (size_t i = 2; i < 4; i++)
			{
				s_RendererData.ShadowsRenderTarget[i] = FrameBuffer::Create(shadowMapSpecs);
			}
		}
		else
		{
			auto& shadowMapSpecs = s_RendererData.ShadowsRenderTarget[0]->GetSpecifications();
			if (shadowMapSpecs.Width != s_RendererData.ShadowMappingSettings.Resolution
				|| shadowMapSpecs.Height != s_RendererData.ShadowMappingSettings.Resolution)
			{
				for (size_t i = 0; i < 4; i++)
				{
					uint32_t resolution = s_RendererData.ShadowMappingSettings.Resolution;
					if (i >= 2)
						resolution /= 2;

					s_RendererData.ShadowsRenderTarget[i]->Resize(resolution, resolution);
				}
			}
		}

		for (size_t i = 0; i < s_RendererData.ShadowMappingSettings.Cascades; i++)
		{
			s_RendererData.ShadowsRenderTarget[i]->Bind();
			RenderCommand::Clear();
		}
		viewport.RenderTarget->Bind();

		// Generate camera frustum planes
		
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

		// Generate camera frustum planes
		{
			FrustumPlanes& frustumPlanes = viewport.FrameData.CameraFrustumPlanes;

			// Near
			frustumPlanes.Planes[FrustumPlanes::NearPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[0], s_RendererData.CurrentViewport->FrameData.Camera.ViewDirection);

			// Far
			frustumPlanes.Planes[FrustumPlanes::FarPlaneIndex] = Math::Plane::TroughPoint(frustumCorners[4], -s_RendererData.CurrentViewport->FrameData.Camera.ViewDirection);

			// Left (Trough bottom left corner)
			frustumPlanes.Planes[2] = Math::Plane::TroughPoint(frustumCorners[0], glm::normalize(glm::cross(
				// Bottom Left Near -> Bottom Left Far
				(glm::vec3)(frustumCorners[4] - frustumCorners[0]),
				// Bottom Left Near -> Top Left Near
				(glm::vec3)(frustumCorners[2] - frustumCorners[0]))));

			// Right (Trough top right corner)
			frustumPlanes.Planes[3] = Math::Plane::TroughPoint(frustumCorners[1], glm::cross(
				// Top Right Near -> Top Right Far
				(glm::vec3)(frustumCorners[7] - frustumCorners[3]),
				// Top Right Near -> Bottom Right Near
				(glm::vec3)(frustumCorners[1] - frustumCorners[3])));

			// Top (Trough top right corner)
			frustumPlanes.Planes[4] = Math::Plane::TroughPoint(frustumCorners[3], glm::cross(
				// Top Right Near -> Top Left Near
				(glm::vec3)(frustumCorners[2] - frustumCorners[3]),
				// Top Right Near -> Top Right Far
				(glm::vec3)(frustumCorners[7] - frustumCorners[3])));

			// Bottom (Trough bottom left corner)
			frustumPlanes.Planes[5] = Math::Plane::TroughPoint(frustumCorners[0], glm::cross(
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

		if ((uint64_t)a.Material->Handle < (uint64_t)b.Material->Handle)
			return true;

		if (a.Material->Handle == b.Material->Handle)
		{
			if ((uint64_t)a.Mesh->Handle < (uint64_t)b.Mesh->Handle)
				return true;
		}

		return false;
	}

	static void PerformFrustumCulling()
	{
		std::array<glm::vec3, 8> aabbCorners;
		Math::AABB objectAABB;

		const FrustumPlanes& planes = s_RendererData.CurrentViewport->FrameData.CameraFrustumPlanes;
		for (size_t i = 0; i < s_RendererData.Queue.size(); i++)
		{
			const RenderableObject& object = s_RendererData.Queue[i];
			const Math::AABB& meshBounds = object.Mesh->GetSubMesh().Bounds;

			meshBounds.GetCorners(aabbCorners.data());

			for (size_t i = 0; i < 8; i++)
			{
				aabbCorners[i] = (glm::vec3)(object.Transform * glm::vec4(aabbCorners[i], 1.0f));
			}

			bool intersectsFrustum = false;

			objectAABB.Min = aabbCorners[0];
			objectAABB.Max = aabbCorners[0];

			for (size_t i = 1; i < 8; i++)
			{
				objectAABB.Min = glm::min(objectAABB.Min, aabbCorners[i]);
				objectAABB.Max = glm::max(objectAABB.Max, aabbCorners[i]);
			}

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
		PerformFrustumCulling();

		std::sort(s_RendererData.CulledObjectIndices.begin(), s_RendererData.CulledObjectIndices.end(), CompareRenderableObjects);

		// Bind white texture for each cascade
		for (size_t i = 0; i < 4; i++)
		{
			s_RendererData.WhiteTexture->Bind(2 + (uint32_t)i);
		}

		s_RendererData.RandomAngles->Bind(6);

		RenderCommand::SetDepthTestEnabled(true);
		RenderCommand::SetCullingMode(CullingMode::Front);

		for (size_t i = 0; i < s_RendererData.ShadowMappingSettings.Cascades; i++)
		{
			const FrameBufferSpecifications& shadowMapSpecs = s_RendererData.ShadowsRenderTarget[i]->GetSpecifications();
			RenderCommand::SetViewport(0, 0, shadowMapSpecs.Width, shadowMapSpecs.Height);

			s_RendererData.CameraBuffer->SetData(
				&s_RendererData.CurrentViewport->FrameData.LightView[i],
				sizeof(s_RendererData.CurrentViewport->FrameData.LightView[i]), 0);

			s_RendererData.ShadowsRenderTarget[i]->Bind();

			DrawQueued(true);
		}

		RenderCommand::SetViewport(0, 0, s_RendererData.CurrentViewport->GetSize().x, s_RendererData.CurrentViewport->GetSize().y);
		s_RendererData.CurrentViewport->RenderTarget->Bind();

		s_RendererData.CameraBuffer->SetData(
			&s_RendererData.CurrentViewport->FrameData.Camera,
			sizeof(s_RendererData.CurrentViewport->FrameData.Camera), 0);

		FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();
		
		for (size_t i = 0; i < 4; i++)
		{
			s_RendererData.ShadowsRenderTarget[i]->BindAttachmentTexture(0, 2 + (uint32_t)i);
		}

		DrawQueued(false);
		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);

		s_RendererData.InstanceDataBuffer.clear();
		s_RendererData.CulledObjectIndices.clear();
		s_RendererData.Queue.clear();
	}

	void Renderer::DrawQueued(bool shadowPass)
	{
		Ref<Material> currentMaterial = nullptr;

		for (uint32_t objectIndex : s_RendererData.CulledObjectIndices)
		{
			const RenderableObject& object = s_RendererData.Queue[objectIndex];

			if (shadowPass && HAS_BIT(object.Flags, MeshRenderFlags::DontCastShadows))
				continue;

			if (object.Mesh->GetSubMesh().InstanceBuffer == nullptr)
				object.Mesh->SetInstanceBuffer(s_RendererData.InstanceBuffer);

			if (s_RendererData.CurrentInstancingMesh.get() != object.Mesh.get())
			{
				FlushInstances();
				s_RendererData.CurrentInstancingMesh = object.Mesh;
			}

			if (object.Material.get() != currentMaterial.get())
			{
				FlushInstances();

				currentMaterial = object.Material;

				if (!shadowPass)
					ApplyMaterialFeatures(object.Material->GetShader()->GetFeatures());

				currentMaterial->SetShaderProperties();

				if (!shadowPass)
				{
					Ref<Shader> shader = object.Material->GetShader();
					if (shader == nullptr)
						continue;

					FrameBufferAttachmentsMask shaderOutputsMask = 0;
					for (uint32_t output : shader->GetOutputs())
						shaderOutputsMask |= (1 << output);

					s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(shaderOutputsMask);
				}
			}

			auto& instanceData = s_RendererData.InstanceDataBuffer.emplace_back();
			instanceData.Transform = object.Transform;
			instanceData.EntityIndex = object.EntityIndex;

			if (s_RendererData.InstanceDataBuffer.size() == (size_t)s_RendererData.MaxInstances)
				FlushInstances();
		}

		FlushInstances();
		s_RendererData.CurrentInstancingMesh = nullptr;
	}

	void Renderer::FlushInstances()
	{
		size_t instancesCount = s_RendererData.InstanceDataBuffer.size();
		if (instancesCount == 0 || s_RendererData.CurrentInstancingMesh == nullptr)
			return;

		s_RendererData.InstanceBuffer->SetData(s_RendererData.InstanceDataBuffer.data(), sizeof(InstanceData) * instancesCount);

		RenderCommand::DrawInstanced(s_RendererData.CurrentInstancingMesh->GetSubMesh().MeshVertexArray, instancesCount);
		s_RendererData.Statistics.DrawCallsCount++;
		s_RendererData.Statistics.DrawCallsSavedByInstances += (uint32_t)instancesCount - 1;

		s_RendererData.InstanceDataBuffer.clear();
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::DrawFullscreenQuad(const Ref<Material>& material)
	{
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
		material->SetShaderProperties();
		ApplyMaterialFeatures(material->GetShader()->GetFeatures());

		const ShaderOutputs& shaderOutputs = material->GetShader()->GetOutputs();

		FrameBufferAttachmentsMask shaderOutputsMask = 0;
		for (uint32_t output : shaderOutputs)
			shaderOutputsMask |= (1 << output);

		FrameBufferAttachmentsMask previousMask = s_RendererData.CurrentViewport->RenderTarget->GetWriteMask();
		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(shaderOutputsMask);

		RenderCommand::DrawIndexed(mesh, indicesCount == SIZE_MAX ? mesh->GetIndexBuffer()->GetCount() : indicesCount);
		s_RendererData.Statistics.DrawCallsCount++;

		s_RendererData.CurrentViewport->RenderTarget->SetWriteMask(previousMask);
	}

	void Renderer::DrawMesh(const Ref<Mesh>& mesh, const Ref<Material>& material, const glm::mat4& transform, MeshRenderFlags flags, int32_t entityIndex)
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
