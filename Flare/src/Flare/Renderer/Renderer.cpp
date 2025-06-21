#include "PCH.h"

#include "Renderer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/ECSContext.h"
#include "FlareECS/World.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Math/AffineTransform.h"
#include "Flare/Math/SIMD.h"

#include "Flare/DebugRenderer/DebugRenderer.h"
#include "Flare/Renderer/DescriptorSet.h"
#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Material.h"
#include "Flare/Renderer/Passes/DecalsPass.h"
#include "Flare/Renderer/Passes/GeometryPass.h"
#include "Flare/Renderer/Passes/GeometryCullingPass.h"
#include "Flare/Renderer/Passes/ShadowPass.h"
#include "Flare/Renderer/Passes/ShadowCascadePass.h"
#include "Flare/Renderer/Passes/SpotLightShadowPass.h"
#include "Flare/Renderer/PostProcessing/PostProcessingManager.h"
#include "Flare/Renderer/RendererComponents.h"
#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/Sampler.h"
#include "Flare/Renderer/SceneSubmition.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/Project/Project.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanGPUTimer.h"

namespace Flare
{
	FLARE_IMPL_TYPE(ShadowSettings);
	FLARE_SERIALIZABLE_IMPL(ShadowSettings);

	struct RendererData
	{
		SceneSubmition* Submition = nullptr;

		Ref<Texture> WhiteTexture = nullptr;
		Ref<Texture> BlackTexture = nullptr;

		Ref<Texture> DefaultNormalMap = nullptr;
		Ref<Texture> DummyDepthTexture = nullptr;

		Ref<Material> ErrorMaterial = nullptr;
		Ref<Material> DepthOnlyMeshMaterial = nullptr;
		
		RendererStatistics Statistics;

		Ref<Sampler> DefaultShadowSampler = nullptr;
		Ref<Sampler> DefaultLinearClampSampler = nullptr;
		Ref<Sampler> DefaultClosestClampSampler = nullptr;

		// Shadows
		ShadowSettings ShadowMappingSettings;

		// Lighting
		Ref<DescriptorSetPool> CameraDescriptorSetPool = nullptr;
		Ref<DescriptorSetPool> GlobalDescriptorSetPool = nullptr;
		Ref<DescriptorSetPool> InstanceDataDescriptorSetPool = nullptr;

		// Decals
		Ref<DescriptorSetPool> DecalsDescriptorSetPool = nullptr;

		// Render World
		Scope<World> RenderWorld = nullptr;
		Scope<Query> ViewportsQuery;
	};
	
	RendererData s_RendererData;

	void Renderer::ReloadShaders()
	{
		std::optional<AssetHandle> errorShaderHandle = ShaderLibrary::FindShader("Error");
		if (errorShaderHandle && AssetManager::IsAssetHandleValid(*errorShaderHandle))
		{
			s_RendererData.ErrorMaterial = Material::Create(AssetManager::GetAsset<Shader>(*errorShaderHandle));
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
		FLARE_PROFILE_FUNCTION();
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

		{
			uint32_t pixel = 0xff000000;
			s_RendererData.BlackTexture = Texture::Create(1, 1, &pixel, TextureFormat::RGBA8);
			s_RendererData.BlackTexture->SetDebugName("Black");
		}

		{
			TextureSpecifications specifications{};
			specifications.Width = 1;
			specifications.Height = 1;
			specifications.Format = TextureFormat::Depth32;
			specifications.Filtering = TextureFiltering::Closest;
			specifications.Wrap = TextureWrap::Clamp;
			specifications.Usage = TextureUsage::Sampling | TextureUsage::RenderTarget;
			s_RendererData.DummyDepthTexture = Texture::Create(specifications);
			s_RendererData.DummyDepthTexture->SetDebugName("DummyDepthTexture");

			Ref<VulkanTexture> dummyDepthTexture = s_RendererData.DummyDepthTexture.As<VulkanTexture>();

			Ref<VulkanCommandBuffer> commandBuffer = VulkanContext::GetInstance().BeginTemporaryCommandBuffer();
			commandBuffer->ClearDepth(s_RendererData.DummyDepthTexture, 1.0f);
			commandBuffer->TransitionDepthImageLayout(
				dummyDepthTexture->GetImageHandle(),
				HasStencilComponent(specifications.Format),
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			VulkanContext::GetInstance().EndTemporaryCommandBuffer(commandBuffer);
		}

		if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
		{
			{
				VkDescriptorSetLayoutBinding bindings[4 + 4 + 4 + 1 + 1 + 1] = {};
				auto& shadowDataBinding = bindings[0];
				shadowDataBinding.binding = 0;
				shadowDataBinding.descriptorCount = 1;
				shadowDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				shadowDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

				auto& lightDataBinding = bindings[1];
				lightDataBinding.binding = 1;
				lightDataBinding.descriptorCount = 1;
				lightDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				lightDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

				auto& pointLightsBinding = bindings[2];
				pointLightsBinding.binding = 2;
				pointLightsBinding.descriptorCount = 1;
				pointLightsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				pointLightsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

				auto& spotLightsBinding = bindings[3];
				spotLightsBinding.binding = 3;
				spotLightsBinding.descriptorCount = 1;
				spotLightsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				spotLightsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

				for (uint32_t i = 0; i < ShadowSettings::MaxCascades; i++)
				{
					auto& cascadeBinding = bindings[i + 4];
					cascadeBinding.binding = i + 4;
					cascadeBinding.descriptorCount = 1;
					cascadeBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					cascadeBinding.pImmutableSamplers = nullptr;
					cascadeBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				}

				for (uint32_t i = 0; i < ShadowSettings::MaxCascades; i++)
				{
					auto& cascadeBinding = bindings[i + 8];
					cascadeBinding.binding = i + 8;
					cascadeBinding.descriptorCount = 1;
					cascadeBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					cascadeBinding.pImmutableSamplers = nullptr;
					cascadeBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				}

				auto& aoTexture = bindings[12];
				aoTexture.binding = 12;
				aoTexture.descriptorCount = 1;
				aoTexture.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				aoTexture.pImmutableSamplers = nullptr;
				aoTexture.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				auto& spotLightShadowMap = bindings[13];
				spotLightShadowMap.binding = 13;
				spotLightShadowMap.descriptorCount = 1;
				spotLightShadowMap.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				spotLightShadowMap.pImmutableSamplers = nullptr;
				spotLightShadowMap.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				auto& spotLightShadowData = bindings[14];
				spotLightShadowData.binding = 14;
				spotLightShadowData.descriptorCount = 1;
				spotLightShadowData.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				spotLightShadowData.pImmutableSamplers = nullptr;
				spotLightShadowData.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

				s_RendererData.GlobalDescriptorSetPool = Ref<VulkanDescriptorSetPool>::New(Span(bindings, 15));
			}

			{
				VkDescriptorSetLayoutBinding cameraBinding{};
				cameraBinding.binding = 0;
				cameraBinding.descriptorCount = 1;
				cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				cameraBinding.pImmutableSamplers = nullptr;
				cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

				s_RendererData.CameraDescriptorSetPool = Ref<VulkanDescriptorSetPool>::New(Span(&cameraBinding, 1));
			}

			{
				VkDescriptorSetLayoutBinding instanceDataBinding{};
				instanceDataBinding.binding = 0;
				instanceDataBinding.descriptorCount = 1;
				instanceDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				instanceDataBinding.pImmutableSamplers = nullptr;
				instanceDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

				s_RendererData.InstanceDataDescriptorSetPool = Ref<VulkanDescriptorSetPool>::New(Span(&instanceDataBinding, 1));
			}

			// Decals descriptor set
			VkDescriptorSetLayoutBinding decalDepthBinding{};
			decalDepthBinding.binding = 0;
			decalDepthBinding.descriptorCount = 1;
			decalDepthBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			decalDepthBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			s_RendererData.DecalsDescriptorSetPool = Ref<VulkanDescriptorSetPool>::New(Span(&decalDepthBinding, 1));
		}

		{
			SamplerSpecifications samplerSpecifications{};
			samplerSpecifications.ComparisonEnabled = true;
			samplerSpecifications.ComparisonFunction = DepthComparisonFunction::Less;
			samplerSpecifications.Filter = TextureFiltering::Linear;
			samplerSpecifications.WrapMode = TextureWrap::Clamp;

			s_RendererData.DefaultShadowSampler = Sampler::Create(samplerSpecifications);
		}

		{
			SamplerSpecifications samplerSpecifications{};
			samplerSpecifications.ComparisonEnabled = false;
			samplerSpecifications.Filter = TextureFiltering::Linear;
			samplerSpecifications.WrapMode = TextureWrap::Clamp;

			s_RendererData.DefaultLinearClampSampler = Sampler::Create(samplerSpecifications);
		}

		{
			SamplerSpecifications samplerSpecifications{};
			samplerSpecifications.ComparisonEnabled = false;
			samplerSpecifications.Filter = TextureFiltering::Closest;
			samplerSpecifications.WrapMode = TextureWrap::Clamp;

			s_RendererData.DefaultClosestClampSampler = Sampler::Create(samplerSpecifications);
		}

		RendererPrimitives::Initialize();

		Project::OnProjectOpen.Bind(ReloadShaders);

		s_RendererData.RenderWorld = CreateScope<World>(ECSContext::GetGlobal());
	}

	void Renderer::Shutdown()
	{
		s_RendererData = {};
	}

	const RendererStatistics& Renderer::GetStatistics()
	{
		return s_RendererData.Statistics;
	}

	void Renderer::ClearStatistics()
	{
		s_RendererData.Statistics = {};
	}

	void Renderer::BeginFrame()
	{
	}

	void Renderer::EndFrame()
	{
	}

	SceneSubmition& Renderer::GetCurrentSceneSubmition()
	{
		FLARE_CORE_ASSERT(s_RendererData.Submition);
		return *s_RendererData.Submition;
	}

	void Renderer::BeginScene(SceneSubmition& sceneSubmition)
	{
		FLARE_CORE_ASSERT(s_RendererData.Submition == nullptr);

		s_RendererData.Submition = &sceneSubmition;
	}

	void Renderer::EndScene()
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(s_RendererData.Submition);
		s_RendererData.Submition = nullptr;
	}

	Ref<Texture> Renderer::GetWhiteTexture()
	{
		return s_RendererData.WhiteTexture;
	}

	Ref<Texture> Renderer::GetBlackTexture()
	{
		return s_RendererData.BlackTexture;
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

	Ref<DescriptorSetPool> Renderer::GetGlobalDescriptorSetPool()
	{
		return s_RendererData.GlobalDescriptorSetPool;
	}

	Ref<DescriptorSetPool> Renderer::GetCameraDescriptorSetPool()
	{
		return s_RendererData.CameraDescriptorSetPool;
	}

	Ref<DescriptorSetPool> Renderer::GetInstanceDataDescriptorSetPool()
	{
		return s_RendererData.InstanceDataDescriptorSetPool;
	}

	const ShadowSettings& Renderer::GetShadowSettings()
	{
		return s_RendererData.ShadowMappingSettings;
	}

	void Renderer::SetShadowSettings(const ShadowSettings& settings)
	{
		bool rebuildRenderGraph = settings.Quality != s_RendererData.ShadowMappingSettings.Quality;
		rebuildRenderGraph |= settings.Enabled != s_RendererData.ShadowMappingSettings.Enabled;

		if (rebuildRenderGraph)
			RequestRenderGraphRebuilds();

		s_RendererData.ShadowMappingSettings = settings;
	}

	Ref<Sampler> Renderer::GetDefaultShadowSampler()
	{
		return s_RendererData.DefaultShadowSampler;
	}

	Ref<const DescriptorSetLayout> Renderer::GetDecalsDescriptorSetLayout()
	{
		return s_RendererData.DecalsDescriptorSetPool->GetLayout();
	}

	static void SetupGlobalDescriptorSet(Ref<DescriptorSet> set)
	{
		FLARE_PROFILE_FUNCTION();
		for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
		{
			set->WriteImage(s_RendererData.DummyDepthTexture, (uint32_t)(4 + i));
		}

		for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
		{
			set->WriteImage(s_RendererData.DummyDepthTexture, s_RendererData.DefaultShadowSampler, (uint32_t)(8 + i));
		}

		// Set AO texture
		set->WriteImage(s_RendererData.WhiteTexture, s_RendererData.DefaultClosestClampSampler, 12);
		set->WriteImage(s_RendererData.DummyDepthTexture, s_RendererData.DefaultShadowSampler, 13);

		set->FlushWrites();
	}

	static void SetupInitialGlobalDescriptorSets(Entity viewportEntity)
	{
		FLARE_PROFILE_FUNCTION();

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		const ViewportGlobalResources& viewportResources = s_RendererData.RenderWorld->GetEntityComponent<const ViewportGlobalResources>(viewportEntity);

		for (uint32_t i = 0; i < frameInFlightCount; i++)
		{
			const ViewportFrameResources& viewportFrameResources = viewportResources.FrameResources[i];
			SetupGlobalDescriptorSet(viewportFrameResources.GlobalDescriptorSet);
			SetupGlobalDescriptorSet(viewportFrameResources.GlobalDescriptorSetWithoutShadows);
		}
	}

	static Ref<ShadowPass> ConfigureShadowPass(Entity viewportEntity,
		const Viewport& viewport,
		RenderGraph& renderGraph,
		std::array<RenderGraphTextureId, ShadowSettings::MaxCascades>& cascadeTextures)
	{
		FLARE_PROFILE_FUNCTION();

		RenderGraphPassSpecifications shadowPassSpec{};
		shadowPassSpec.SetDebugName("ShadowPass");

		Ref<ShadowPass> shadowPass = Ref<ShadowPass>::New();

		renderGraph.AddPass(shadowPassSpec, shadowPass);

		if (!viewport.Settings.ShadowMappingEnabled)
			return shadowPass;

		uint32_t shadowTextureResolution = GetShadowMapResolution(s_RendererData.ShadowMappingSettings.Quality);
		for (int32_t cascadeIndex = 0; cascadeIndex < s_RendererData.ShadowMappingSettings.Cascades; cascadeIndex++)
		{
			cascadeTextures[cascadeIndex] = renderGraph.GetResourceManager().CreateFixedSizeTexture(
				TextureFormat::Depth32,
				glm::uvec2(shadowTextureResolution),
				fmt::format("CascadeTexture.{}", cascadeIndex),
				RenderGraphTextureAllocationMode::Preallocated);

			RenderGraphPassSpecifications cascadePassSpec{};
			cascadePassSpec.SetDebugName(fmt::format("ShadowCascadePass{}", cascadeIndex));
			cascadePassSpec.AddOutput(cascadeTextures[cascadeIndex], 1.0f);

			Ref<ShadowCascadePass> cascadePass = Ref<ShadowCascadePass>::New(
				s_RendererData.Statistics,
				shadowPass->GetCascadeData((size_t)cascadeIndex),
				shadowPass->GetFilteredTransforms(),
				shadowPass->GetVisibleSubMeshIndices());

			renderGraph.AddPass(cascadePassSpec, cascadePass);
		}

		// Update global descriptor sets
		const ViewportRenderGraph& viewportRenderGraph = s_RendererData.RenderWorld->GetEntityComponent<const ViewportRenderGraph>(viewportEntity);
		const ViewportGlobalResources& viewportResources = s_RendererData.RenderWorld->GetEntityComponent<const ViewportGlobalResources>(viewportEntity);
		
		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		const RenderGraphResourceManager& resourceManager = viewportRenderGraph.Graph->GetResourceManager();
		for (uint32_t frameIndex = 0; frameIndex < frameInFlightCount; frameIndex++)
		{
			const ViewportFrameResources& viewportFrameResources = viewportResources.FrameResources[frameIndex];
			Ref<DescriptorSet> set = viewportFrameResources.GlobalDescriptorSet;
			for (uint32_t cascadeIndex = 0; cascadeIndex < (uint32_t)Renderer::GetShadowSettings().Cascades; cascadeIndex++)
			{
				Ref<Texture> cascadeTexture = resourceManager.GetTextureForFrameInFlight(cascadeTextures[cascadeIndex], frameIndex);
				set->WriteImage(cascadeTexture, 4 + cascadeIndex);
				set->WriteImage(cascadeTexture, s_RendererData.DefaultShadowSampler, 8 + cascadeIndex);
			}

			set->FlushWrites();
		}

		return shadowPass;
	}

	static RenderGraphTextureId ConfigureSpotLightShadowPass(Entity viewportEntity, RenderGraph& renderGraph, Ref<Material> perspectiveDepthOnly)
	{
		FLARE_PROFILE_FUNCTION();

		// TODO: Store this somewhere
		SpotLightShadowsSpecifications shadowSpecifications{};

		RenderGraphTextureId shadowMapId = renderGraph.GetResourceManager().CreateFixedSizeTexture(
			TextureFormat::Depth32,
			glm::uvec2(1 << shadowSpecifications.SizeLog2),
			"SpotLightShadowMap");

		Ref<SpotLightShadowPass> pass = Ref<SpotLightShadowPass>::New(shadowMapId, perspectiveDepthOnly, shadowSpecifications);

		const ViewportDepthOutput& depthOutput = s_RendererData.RenderWorld->GetEntityComponent<const ViewportDepthOutput>(viewportEntity);

		RenderGraphPassSpecifications specifications{};
		specifications.SetDebugName("SpotLightShadowPass");
		specifications.AddOutput(shadowMapId, 1.0f);
		renderGraph.AddPass(specifications, pass);

		const ViewportRenderGraph& viewportRenderGraph = s_RendererData.RenderWorld->GetEntityComponent<const ViewportRenderGraph>(viewportEntity);
		const ViewportGlobalResources& viewportResources = s_RendererData.RenderWorld->GetEntityComponent<const ViewportGlobalResources>(viewportEntity);

		uint32_t frameInFlightCount = GraphicsContext::GetInstance().GetFrameInFlightCount();
		const RenderGraphResourceManager& resourceManager = viewportRenderGraph.Graph->GetResourceManager();
		for (uint32_t frameIndex = 0; frameIndex < frameInFlightCount; frameIndex++)
		{
			const ViewportFrameResources& viewportFrameResources = viewportResources.FrameResources[frameIndex];
			Ref<DescriptorSet> set = viewportFrameResources.GlobalDescriptorSet;
			Ref<Texture> texture = resourceManager.GetTextureForFrameInFlight(shadowMapId, frameIndex);
			set->WriteImage(texture, s_RendererData.DefaultShadowSampler, 13);
			set->FlushWrites();
		}

		return shadowMapId;
	}

	static Ref<Material> CreateDepthPrepassMaterial()
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<AssetHandle> shaderHandle = ShaderLibrary::FindShader("DepthOnlyPerspective");
		if (shaderHandle && AssetManager::IsAssetHandleValid(*shaderHandle))
		{
			return Material::Create(*shaderHandle);
		}

		return nullptr;
	}

	static void ConfigureDepthPrepass(Entity viewportEntity, Ref<Material> depthPrepassMaterial)
	{
		FLARE_PROFILE_FUNCTION();

		ViewportRenderGraph* viewportRenderGraph = s_RendererData.RenderWorld->TryGetEntityComponent<ViewportRenderGraph>(viewportEntity);
		FLARE_CORE_ASSERT(viewportRenderGraph);

		const ViewportDepthOutput* depthOutput = s_RendererData.RenderWorld->TryGetEntityComponent<const ViewportDepthOutput>(viewportEntity);
		FLARE_CORE_ASSERT(depthOutput);

		// Culling pass
		{
			RenderGraphPassSpecifications geometryCullingPass{};
			geometryCullingPass.SetDebugName("GeometryCullingPass");
			geometryCullingPass.SetType(RenderGraphPassType::Other);

			viewportRenderGraph->Graph->AddPass(geometryCullingPass, Ref<GeometryCullingPass>::New());
		}
	
		// Depth prepass
		{
			RenderGraphPassSpecifications depthPrepass{};
			depthPrepass.SetDebugName("DepthPrepass");
			depthPrepass.SetType(RenderGraphPassType::Graphics);
			depthPrepass.AddOutput(depthOutput->Id);

			viewportRenderGraph->Graph->AddPass(depthPrepass, Ref<GeometryPass>::New(s_RendererData.Statistics, depthPrepassMaterial, true));
		}
	}

	static void ConfigureGeometryPass(Entity viewportEntity,
		const std::array<RenderGraphTextureId, ShadowSettings::MaxCascades>& cascadeTextures,
		RenderGraphTextureId spotLightShadowMap)
	{
		FLARE_PROFILE_FUNCTION();

		const Viewport* viewport = s_RendererData.RenderWorld->TryGetEntityComponent<const Viewport>(viewportEntity);
		ViewportRenderGraph* viewportRenderGraph = s_RendererData.RenderWorld->TryGetEntityComponent<ViewportRenderGraph>(viewportEntity);
		FLARE_CORE_ASSERT(viewport && viewportRenderGraph);

		const ViewportColorOutput* colorOutput = s_RendererData.RenderWorld->TryGetEntityComponent<const ViewportColorOutput>(viewportEntity);
		const ViewportDepthOutput* depthOutput = s_RendererData.RenderWorld->TryGetEntityComponent<const ViewportDepthOutput>(viewportEntity);
		FLARE_CORE_ASSERT(colorOutput && depthOutput);

		RenderGraphPassSpecifications geometryPass{};
		geometryPass.SetDebugName("GeometryPass");
		geometryPass.AddOutput(colorOutput->Id);
		geometryPass.AddOutput(depthOutput->Id);

		if (viewport->Settings.ShadowMappingEnabled)
		{
			for (size_t i = 0; i < cascadeTextures.size(); i++)
			{
				if (cascadeTextures[i].GetValue() == UINT32_MAX)
					break;

				geometryPass.AddInput(cascadeTextures[i]);
			}
		}

		// Setup AO input
		const AOConfiguration& aoConfiguration = s_RendererData.RenderWorld->GetEntityComponent<const AOConfiguration>(viewportEntity);
		if (viewportRenderGraph->Graph->GetResourceManager().IsTextureIdValid(aoConfiguration.AOTexture))
		{
			geometryPass.AddInput(aoConfiguration.AOTexture);

			// Update descriptor sets with the ao texture

			const ViewportGlobalResources& viewportResources = s_RendererData.RenderWorld->GetEntityComponent<const ViewportGlobalResources>(viewportEntity);
			const RenderGraphResourceManager& resourceManager = viewportRenderGraph->Graph->GetResourceManager();
			for (uint32_t frameInFlight = 0; frameInFlight < GraphicsContext::GetInstance().GetFrameInFlightCount(); frameInFlight++)
			{
				const ViewportFrameResources& currentFrameResources = viewportResources.FrameResources[frameInFlight];

				Ref<Texture> aoTexture = resourceManager.GetTextureForFrameInFlight(aoConfiguration.AOTexture, frameInFlight);

				currentFrameResources.GlobalDescriptorSet->WriteImage(aoTexture, s_RendererData.DefaultClosestClampSampler, 12);
				currentFrameResources.GlobalDescriptorSet->FlushWrites();

				currentFrameResources.GlobalDescriptorSetWithoutShadows->WriteImage(aoTexture, s_RendererData.DefaultClosestClampSampler, 12);
				currentFrameResources.GlobalDescriptorSetWithoutShadows->FlushWrites();
			}
		}

		geometryPass.AddInput(spotLightShadowMap);

		viewportRenderGraph->Graph->AddPass(geometryPass, Ref<GeometryPass>::New(s_RendererData.Statistics, nullptr, false));
	}

	static void ConfigureOtherPasses(Entity viewportEntity)
	{
		FLARE_PROFILE_FUNCTION();

		ViewportRenderGraph* viewportRenderGraph = s_RendererData.RenderWorld->TryGetEntityComponent<ViewportRenderGraph>(viewportEntity);
		FLARE_CORE_ASSERT(viewportRenderGraph);

		const ViewportColorOutput* colorOutput = s_RendererData.RenderWorld->TryGetEntityComponent<const ViewportColorOutput>(viewportEntity);
		const ViewportDepthOutput* depthOutput = s_RendererData.RenderWorld->TryGetEntityComponent<const ViewportDepthOutput>(viewportEntity);
		FLARE_CORE_ASSERT(colorOutput && depthOutput);

		// Decal pass
		RenderGraphPassSpecifications decalPass{};
		decalPass.AddInput(depthOutput->Id);
		decalPass.AddOutput(colorOutput->Id);
		decalPass.SetDebugName("DecalsPass");

		viewportRenderGraph->Graph->AddPass(decalPass, Ref<DecalsPass>::New(
			s_RendererData.DecalsDescriptorSetPool,
			depthOutput->Id));
	}

	World& Renderer::GetRenderWorld()
	{
		return *s_RendererData.RenderWorld;
	}

	Query& Renderer::GetViewportsQuery()
	{
		if (!s_RendererData.ViewportsQuery)
		{
			s_RendererData.ViewportsQuery = CreateScope<Query>(s_RendererData.RenderWorld->NewQuery()
				.All().With<Viewport, ViewportRenderGraph>().Build());
		}

		return *s_RendererData.ViewportsQuery;
	}

	Entity Renderer::CreateViewport(std::string_view name)
	{
		FLARE_PROFILE_FUNCTION();
		Entity viewport = s_RendererData.RenderWorld->CreateEntity(Viewport { .Name = std::string(name) },
			ViewportRenderGraph(),
			ViewportGlobalResources(),
			ViewportColorOutput(),
			ViewportDepthOutput(),
			AOConfiguration());

		s_RendererData.RenderWorld->AddDefaultEntityComponent<CulledGeometry>(viewport);

		ViewportRenderGraph& renderGraph = s_RendererData.RenderWorld->GetEntityComponent<ViewportRenderGraph>(viewport);
		renderGraph.Graph = RenderGraph::Create(*s_RendererData.RenderWorld, viewport);
		renderGraph.Graph->SetNeedsRebuilding();

		ViewportGlobalResources& viewportResources = s_RendererData.RenderWorld->GetEntityComponent<ViewportGlobalResources>(viewport);
		viewportResources.CreateResources();

		return viewport;
	}

	void Renderer::DeleteViewport(Entity viewportEntity)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(s_RendererData.RenderWorld->IsEntityAlive(viewportEntity));
		FLARE_CORE_ASSERT(s_RendererData.RenderWorld->HasComponent<Viewport>(viewportEntity));

		s_RendererData.RenderWorld->DeleteEntity(viewportEntity);
	}

	static void ResetViewportState(Entity viewportEntity)
	{
		FLARE_PROFILE_FUNCTION();

		s_RendererData.RenderWorld->GetEntityComponent<AOConfiguration>(viewportEntity) = AOConfiguration();
	}

	void Renderer::PrepareViewport(Entity viewportEntity, const std::function<void(RenderGraph&)>& onBuild, PostProcessingManager& postProcessingManager)
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(s_RendererData.RenderWorld->IsEntityAlive(viewportEntity));
		FLARE_CORE_ASSERT(onBuild);

		ViewportRenderGraph* viewportRenderGraph = s_RendererData.RenderWorld->TryGetEntityComponent<ViewportRenderGraph>(viewportEntity);
		FLARE_CORE_ASSERT(viewportRenderGraph);
		FLARE_CORE_ASSERT(viewportRenderGraph->Graph);

		const Viewport* viewport = s_RendererData.RenderWorld->TryGetEntityComponent<const Viewport>(viewportEntity);
		FLARE_CORE_ASSERT(viewport);

		if (!viewport->IsValid())
			return;

		if (viewportRenderGraph->CurrentViewportSize != viewport->Size || viewportRenderGraph->Settings != viewport->Settings)
		{
			viewportRenderGraph->Graph->SetNeedsRebuilding();
		}

		viewportRenderGraph->CurrentViewportSize = viewport->Size;
		viewportRenderGraph->Settings = viewport->Settings;

		ViewportColorOutput* colorOutput = s_RendererData.RenderWorld->TryGetEntityComponent<ViewportColorOutput>(viewportEntity);
		ViewportDepthOutput* depthOutput = s_RendererData.RenderWorld->TryGetEntityComponent<ViewportDepthOutput>(viewportEntity);
		FLARE_CORE_ASSERT(colorOutput && depthOutput);

		if (viewportRenderGraph->Graph->NeedsRebuilding())
		{
			viewportRenderGraph->Graph->Clear();
			ResetViewportState(viewportEntity);

			colorOutput->Id	= viewportRenderGraph->Graph->CreateTexture(TextureFormat::R11G11B10, "Color");
			depthOutput->Id = viewportRenderGraph->Graph->CreateTexture(TextureFormat::Depth32, "Depth");

			ExternalRenderGraphResource colorTextureResource{};
			colorTextureResource.InitialLayout = ImageLayout::AttachmentOutput;
			colorTextureResource.FinalLayout = ImageLayout::ReadOnly;
			colorTextureResource.Texture = colorOutput->Id;
			colorTextureResource.ClearValue = AttachmentClearValue(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

			ExternalRenderGraphResource depthTextureResource{};
			depthTextureResource.InitialLayout = ImageLayout::AttachmentOutput;
			depthTextureResource.FinalLayout = ImageLayout::ReadOnly;
			depthTextureResource.Texture = depthOutput->Id;
			depthTextureResource.ClearValue = AttachmentClearValue(1.0f);

			viewportRenderGraph->Graph->AddExternalResource(colorTextureResource);
			viewportRenderGraph->Graph->AddExternalResource(depthTextureResource);

			{
				SetupInitialGlobalDescriptorSets(viewportEntity);

				std::array<RenderGraphTextureId, ShadowSettings::MaxCascades> cascadeTextures = { RenderGraphTextureId() };
				Ref<ShadowPass> shadowPass = ConfigureShadowPass(viewportEntity, *viewport, *viewportRenderGraph->Graph, cascadeTextures);

				Ref<Material> perspectiveDepthOnly = CreateDepthPrepassMaterial();

				ConfigureDepthPrepass(viewportEntity, perspectiveDepthOnly);

				RenderGraphTextureId spotLightShadowMap = ConfigureSpotLightShadowPass(viewportEntity,
					*viewportRenderGraph->Graph,
					perspectiveDepthOnly);

				if (viewport->Settings.PostProcessingEnabled)
				{
					postProcessingManager.RegisterRenderPasses(*viewportRenderGraph->Graph,
						viewportEntity,
						*s_RendererData.RenderWorld,
						PostProcessingExecutionOrder::AfterDepthPrePass);
				}

				ConfigureGeometryPass(viewportEntity, cascadeTextures, spotLightShadowMap);
				ConfigureOtherPasses(viewportEntity);

				Renderer2D::ConfigurePasses(viewportEntity, *viewportRenderGraph->Graph);

				onBuild(*viewportRenderGraph->Graph);

				if (viewport->Settings.PostProcessingEnabled)
				{
					postProcessingManager.RegisterRenderPasses(*viewportRenderGraph->Graph,
						viewportEntity,
						*s_RendererData.RenderWorld,
						PostProcessingExecutionOrder::EndOfFrame);
				}

				// Debug rendering is done last, to avoid any of the post processing passes overriding the DebugRenderer's output.
				DebugRenderer::ConfigurePasses(*s_RendererData.RenderWorld, *viewportRenderGraph->Graph, viewportEntity);
			}

			viewportRenderGraph->Graph->Build();
		}
		
		FLARE_CORE_ASSERT(viewportRenderGraph->Graph->IsValid());
	}

	void Renderer::RequestRenderGraphRebuilds()
	{
		FLARE_PROFILE_FUNCTION();

		GetViewportsQuery().ForEachChunk([](QueryChunk chunk, ComponentView<ViewportRenderGraph> renderGraphs)
			{
				for (size_t i = 0; i < chunk.GetEntityCount(); i++)
				{
					renderGraphs[i].Graph->SetNeedsRebuilding();
				}
			});
	}
}
