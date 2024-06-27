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
#include "Flare/Renderer/DescriptorSet.h"

#include "Flare/Renderer/Passes/GeometryPass.h"
#include "Flare/Renderer/Passes/ShadowPass.h"
#include "Flare/Renderer/Passes/ShadowCascadePass.h"

#include "Flare/Project/Project.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanGPUTimer.h"

namespace Flare
{
	FLARE_IMPL_TYPE(ShadowSettings);

	struct RenderPasses
	{
		std::vector<Ref<RenderPass>> BeforeShadowsPasses;
		std::vector<Ref<RenderPass>> BeforeOpaqueGeometryPasses;
		std::vector<Ref<RenderPass>> AfterOpaqueGeometryPasses;
		std::vector<Ref<RenderPass>> PostProcessingPasses;
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
		std::vector<DecalData> Decals;

		// Shadows
		ShadowSettings ShadowMappingSettings;

		// Lighting
		std::vector<PointLightData> PointLights;
		Ref<ShaderStorageBuffer> PointLightsShaderBuffer = nullptr;
		std::vector<SpotLightData> SpotLights;
		Ref<ShaderStorageBuffer> SpotLightsShaderBuffer = nullptr;

		Ref<DescriptorSet> PrimaryDescriptorSet = nullptr;
		Ref<DescriptorSet> PrimaryDescriptorSetWithoutShadows = nullptr;
		Ref<DescriptorSetPool> PrimaryDescriptorPool = nullptr;
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

		s_RendererData.CameraBuffer = UniformBuffer::Create(sizeof(RenderView));
		s_RendererData.LightBuffer = UniformBuffer::Create(sizeof(LightData));

		s_RendererData.PointLightsShaderBuffer = ShaderStorageBuffer::Create(maxPointLights * sizeof(PointLightData));
		s_RendererData.PointLightsShaderBuffer->SetDebugName("PointLightsDataBuffer");

		s_RendererData.SpotLightsShaderBuffer = ShaderStorageBuffer::Create(maxSpotLights * sizeof(SpotLightData));
		s_RendererData.SpotLightsShaderBuffer->SetDebugName("SpotLightsDataBuffer");

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
			s_RendererData.WhiteTexture->SetDebugName("White");
		}

		{
			uint32_t pixel = 0xffff8080;
			s_RendererData.DefaultNormalMap = Texture::Create(1, 1, &pixel, TextureFormat::RGBA8);
			s_RendererData.DefaultNormalMap->SetDebugName("DefaultNormalMap");
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

			s_RendererData.PrimaryDescriptorPool = CreateRef<VulkanDescriptorSetPool>(10 + ShadowSettings::MaxCascades, Span(bindings, 6 + 4));
			s_RendererData.PrimaryDescriptorSet = s_RendererData.PrimaryDescriptorPool->AllocateSet();
			s_RendererData.PrimaryDescriptorSet->SetDebugName("PrimarySet");

			s_RendererData.PrimaryDescriptorSetWithoutShadows = s_RendererData.PrimaryDescriptorPool->AllocateSet();
			s_RendererData.PrimaryDescriptorSetWithoutShadows->SetDebugName("PrimarySetWithoutShadows");

			// Setup primary descriptor set
			s_RendererData.PrimaryDescriptorSet->WriteUniformBuffer(s_RendererData.CameraBuffer, 0);
			s_RendererData.PrimaryDescriptorSet->WriteUniformBuffer(s_RendererData.LightBuffer, 1);
			s_RendererData.PrimaryDescriptorSet->WriteStorageBuffer(s_RendererData.PointLightsShaderBuffer, 4);
			s_RendererData.PrimaryDescriptorSet->WriteStorageBuffer(s_RendererData.SpotLightsShaderBuffer, 5);

			for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
			{
				s_RendererData.PrimaryDescriptorSet->WriteImage(s_RendererData.WhiteTexture, (uint32_t)(28 + i));
			}

			s_RendererData.PrimaryDescriptorSet->FlushWrites();

			// Setup primary descriptor set (without shadows)
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteUniformBuffer(s_RendererData.CameraBuffer, 0);
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteUniformBuffer(s_RendererData.LightBuffer, 1);
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteStorageBuffer(s_RendererData.PointLightsShaderBuffer, 4);
			s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteStorageBuffer(s_RendererData.SpotLightsShaderBuffer, 5);

			for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
			{
				s_RendererData.PrimaryDescriptorSetWithoutShadows->WriteImage(s_RendererData.WhiteTexture, (uint32_t)(28 + i));
			}

			s_RendererData.PrimaryDescriptorSetWithoutShadows->FlushWrites();
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

	void Renderer::Flush()
	{
		FLARE_PROFILE_FUNCTION();

		s_RendererData.Statistics.ObjectsSubmitted += (uint32_t)s_RendererData.OpaqueQueue.GetSize();

#if 0
		if (RendererAPI::GetAPI() != RendererAPI::API::Vulkan)
		{
			ExecuteDecalsPass();
		}
#endif

		s_RendererData.CurrentViewport->Graph.Execute(GraphicsContext::GetInstance().GetCommandBuffer());
	}

	void Renderer::ExecuteDecalsPass()
	{
#if 0
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
				const auto& transform = decal.Transform;
				glm::vec4 translation = transform[3];
				instance.PackedTransform[0] = glm::vec4((glm::vec3)transform[0], translation.x);
				instance.PackedTransform[1] = glm::vec4((glm::vec3)transform[1], translation.y);
				instance.PackedTransform[2] = glm::vec4((glm::vec3)transform[2], translation.z);
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
#endif
	}

	void Renderer::EndScene()
	{
		s_RendererData.PointLights.clear();
		s_RendererData.SpotLights.clear();

		s_RendererData.OpaqueQueue.m_Buffer.clear();
	}

	void Renderer::SubmitPointLight(const PointLightData& light)
	{
		s_RendererData.PointLights.push_back(light);
	}

	void Renderer::SubmitSpotLight(const SpotLightData& light)
	{
		s_RendererData.SpotLights.push_back(light);
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

		RenderingContext context(s_RendererData.CurrentViewport->RenderTarget);
		for (Ref<RenderPass>& pass : passes)
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

	Ref<Texture> Renderer::GetDefaultNormalMap()
	{
		return s_RendererData.DefaultNormalMap;
	}

	Ref<Material> Renderer::GetErrorMaterial()
	{
		return s_RendererData.ErrorMaterial;
	}

	Ref<Material> Renderer::GetDepthOnlyMaterial()
	{
		return s_RendererData.DepthOnlyMeshMaterial;
	}

	ShadowSettings& Renderer::GetShadowSettings()
	{
		return s_RendererData.ShadowMappingSettings;
	}

	Ref<DescriptorSet> Renderer::GetPrimaryDescriptorSet()
	{
		return s_RendererData.PrimaryDescriptorSet;
	}

	Ref<const DescriptorSetLayout> Renderer::GetPrimaryDescriptorSetLayout()
	{
		return s_RendererData.PrimaryDescriptorPool->GetLayout();
	}

	static void SetupPrimaryDescriptorSet(Ref<DescriptorSet> set, Ref<UniformBuffer> shadowData)
	{
		set->WriteUniformBuffer(s_RendererData.CameraBuffer, 0);
		set->WriteUniformBuffer(s_RendererData.LightBuffer, 1);
		set->WriteUniformBuffer(shadowData, 2);
		set->WriteStorageBuffer(s_RendererData.PointLightsShaderBuffer, 4);
		set->WriteStorageBuffer(s_RendererData.SpotLightsShaderBuffer, 5);

		for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
		{
			set->WriteImage(s_RendererData.WhiteTexture, (uint32_t)(28 + i));
		}

		set->FlushWrites();
	}

	static Ref<ShadowPass> ConfigureShadowPass(Viewport& viewport, std::array<Ref<Texture>, ShadowSettings::MaxCascades>& cascadeTextures)
	{
		RenderGraphPassSpecifications shadowPassSpec{};
		shadowPassSpec.SetDebugName("ShadowPass");

		Ref<ShadowPass> shadowPass = CreateRef<ShadowPass>(s_RendererData.OpaqueQueue);

		viewport.Graph.AddPass(shadowPassSpec, shadowPass);

		Ref<UniformBuffer> shadowDataBuffer = shadowPass->GetShadowDataBuffer();

		uint32_t textureResolution = GetShadowMapResolution(s_RendererData.ShadowMappingSettings.Quality);

		TextureSpecifications cascadeSpec{};
		cascadeSpec.Filtering = TextureFiltering::Closest;
		cascadeSpec.Wrap = TextureWrap::Clamp;
		cascadeSpec.Format = TextureFormat::Depth32;
		cascadeSpec.Usage = TextureUsage::RenderTarget | TextureUsage::Sampling;
		cascadeSpec.Width = textureResolution;
		cascadeSpec.Height = textureResolution;

		for (int32_t cascadeIndex = 0; cascadeIndex < s_RendererData.ShadowMappingSettings.Cascades; cascadeIndex++)
		{
			cascadeTextures[cascadeIndex] = Texture::Create(cascadeSpec);
			cascadeTextures[cascadeIndex]->SetDebugName(fmt::format("CascadeTexture.{}", cascadeIndex));

			Ref<DescriptorSet> set = s_RendererData.PrimaryDescriptorPool->AllocateSet();
			set->WriteUniformBuffer(s_RendererData.LightBuffer, 1);
			set->WriteStorageBuffer(s_RendererData.PointLightsShaderBuffer, 4);
			set->WriteStorageBuffer(s_RendererData.SpotLightsShaderBuffer, 5);

			for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
			{
				set->WriteImage(s_RendererData.WhiteTexture, (uint32_t)(28 + i));
			}

			set->FlushWrites();
			set->SetDebugName(fmt::format("Cascade{}.PrimaryDescriptorSet", cascadeIndex));

			RenderGraphPassSpecifications cascadePassSpec{};
			cascadePassSpec.SetDebugName(fmt::format("ShadowCascadePass{}", cascadeIndex));
			cascadePassSpec.AddOutput(cascadeTextures[cascadeIndex], 0, 1.0f);

			Ref<ShadowCascadePass> cascadePass = CreateRef<ShadowCascadePass>(
				s_RendererData.OpaqueQueue,
				shadowPass->GetLightView(cascadeIndex),
				shadowPass->GetVisibleObjects(cascadeIndex),
				cascadeTextures[cascadeIndex],
				shadowDataBuffer,
				set, s_RendererData.PrimaryDescriptorPool);

			viewport.Graph.AddPass(cascadePassSpec, cascadePass);
		}

		return shadowPass;
	}

	void Renderer::ConfigurePasses(Viewport& viewport)
	{
		std::array<Ref<Texture>, ShadowSettings::MaxCascades> cascadeTextures = { nullptr };
		Ref<ShadowPass> shadowPass = ConfigureShadowPass(viewport, cascadeTextures);
		Ref<UniformBuffer> shadowDataBuffer = shadowPass->GetShadowDataBuffer();

		Ref<DescriptorSet> primarySet = s_RendererData.PrimaryDescriptorPool->AllocateSet();
		primarySet->SetDebugName("PrimarySet");
		SetupPrimaryDescriptorSet(primarySet, shadowDataBuffer);

		Ref<DescriptorSet> primarySetWithoutShadows = s_RendererData.PrimaryDescriptorPool->AllocateSet();
		primarySetWithoutShadows->SetDebugName("PrimarySetWithoutShadows");
		SetupPrimaryDescriptorSet(primarySetWithoutShadows, shadowDataBuffer);

		RenderGraphPassSpecifications geometryPass{};
		geometryPass.SetDebugName("GeometryPass");
		geometryPass.AddOutput(viewport.ColorTexture, 0);
		geometryPass.AddOutput(viewport.NormalsTexture, 1);
		geometryPass.AddOutput(viewport.DepthTexture, 2);

		for (size_t i = 0; i < cascadeTextures.size(); i++)
		{
			geometryPass.AddInput(cascadeTextures[i]);

			if (cascadeTextures[i] == nullptr)
				break;
		}

		// Fill first cascades with textures from shadow casacde passes,
		// the rest of cascades were filled with white textures when setting up the descriptor set
		for (uint32_t i = 0; i < (uint32_t)s_RendererData.ShadowMappingSettings.Cascades; i++)
		{
			primarySet->WriteImage(cascadeTextures[i], (uint32_t)(28 + i));
		}

		primarySet->FlushWrites();

		viewport.Graph.AddPass(geometryPass, CreateRef<GeometryPass>(
			s_RendererData.OpaqueQueue,
			primarySet,
			primarySetWithoutShadows,
			s_RendererData.PrimaryDescriptorPool));
	}
}
