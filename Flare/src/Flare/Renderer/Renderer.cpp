#include "Renderer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Math/Transform.h"
#include "Flare/Math/SIMD.h"

#include "Flare/Renderer/RendererPrimitives.h"
#include "Flare/Renderer/SceneSubmition.h"
#include "Flare/Renderer/Viewport.h"
#include "Flare/Renderer/UniformBuffer.h"
#include "Flare/Renderer/ShaderLibrary.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/ShaderStorageBuffer.h"
#include "Flare/Renderer/GPUTimer.h"
#include "Flare/Renderer/DescriptorSet.h"

#include "Flare/Renderer/Passes/GeometryPass.h"
#include "Flare/Renderer/Passes/ShadowPass.h"
#include "Flare/Renderer/Passes/ShadowCascadePass.h"
#include "Flare/Renderer/Passes/DecalsPass.h"

#include "Flare/Project/Project.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Platform/Vulkan/VulkanContext.h"
#include "Flare/Platform/Vulkan/VulkanDescriptorSet.h"
#include "Flare/Platform/Vulkan/VulkanCommandBuffer.h"
#include "Flare/Platform/Vulkan/VulkanGPUTimer.h"

namespace Flare
{
	FLARE_IMPL_TYPE(ShadowSettings);

	struct RendererData
	{
		SceneSubmition* Submition = nullptr;

		Viewport* MainViewport = nullptr;
		Viewport* CurrentViewport = nullptr;

		bool RenderGraphRebuildIsRequired = false;

		Ref<Texture> WhiteTexture = nullptr;
		Ref<Texture> DefaultNormalMap = nullptr;
		Ref<Texture> DummyDepthTexture = nullptr;

		Ref<Material> ErrorMaterial = nullptr;
		Ref<Material> DepthOnlyMeshMaterial = nullptr;
		
		RendererStatistics Statistics;

		// Shadows
		ShadowSettings ShadowMappingSettings;

		// Lighting
		Ref<DescriptorSetPool> CameraDescriptorSetPool = nullptr;
		Ref<DescriptorSetPool> GlobalDescriptorSetPool = nullptr;
		Ref<DescriptorSetPool> InstanceDataDescriptorSetPool = nullptr;

		// Decals
		Ref<DescriptorSetPool> DecalsDescriptorSetPool = nullptr;
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
			TextureSpecifications specifications{};
			specifications.Width = 1;
			specifications.Height = 1;
			specifications.Format = TextureFormat::Depth32;
			specifications.Filtering = TextureFiltering::Closest;
			specifications.Wrap = TextureWrap::Clamp;
			specifications.GenerateMipMaps = false;
			specifications.Usage = TextureUsage::Sampling | TextureUsage::RenderTarget;
			s_RendererData.DummyDepthTexture = Texture::Create(specifications);
			s_RendererData.DummyDepthTexture->SetDebugName("DummyDepthTexture");

			Ref<VulkanTexture> dummyDepthTexture = As<VulkanTexture>(s_RendererData.DummyDepthTexture);

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
				VkDescriptorSetLayoutBinding bindings[4 + 4 + 4] = {};
				auto& shadowDataBinding = bindings[0];
				shadowDataBinding.binding = 0;
				shadowDataBinding.descriptorCount = 1;
				shadowDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				shadowDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

				auto& lightDataBinding = bindings[1];
				lightDataBinding.binding = 1;
				lightDataBinding.descriptorCount = 1;
				lightDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				lightDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

				auto& pointLightsBinding = bindings[2];
				pointLightsBinding.binding = 2;
				pointLightsBinding.descriptorCount = 1;
				pointLightsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				pointLightsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

				auto& spotLightsBinding = bindings[3];
				spotLightsBinding.binding = 3;
				spotLightsBinding.descriptorCount = 1;
				spotLightsBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				spotLightsBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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

				s_RendererData.GlobalDescriptorSetPool = CreateRef<VulkanDescriptorSetPool>(8, Span(bindings, 12));
			}

			{
				VkDescriptorSetLayoutBinding cameraBinding{};
				cameraBinding.binding = 0;
				cameraBinding.descriptorCount = 1;
				cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				cameraBinding.pImmutableSamplers = nullptr;
				cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

				s_RendererData.CameraDescriptorSetPool = CreateRef<VulkanDescriptorSetPool>(32, Span(&cameraBinding, 1));
			}

			{
				VkDescriptorSetLayoutBinding instanceDataBinding{};
				instanceDataBinding.binding = 0;
				instanceDataBinding.descriptorCount = 1;
				instanceDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				instanceDataBinding.pImmutableSamplers = nullptr;
				instanceDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

				s_RendererData.InstanceDataDescriptorSetPool = CreateRef<VulkanDescriptorSetPool>(32, Span(&instanceDataBinding, 1));
			}

			// Decals descriptor set
			VkDescriptorSetLayoutBinding decalDepthBinding{};
			decalDepthBinding.binding = 0;
			decalDepthBinding.descriptorCount = 1;
			decalDepthBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			decalDepthBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			s_RendererData.DecalsDescriptorSetPool = CreateRef<VulkanDescriptorSetPool>(32, Span(&decalDepthBinding, 1));
		}

		Project::OnProjectOpen.Bind(ReloadShaders);
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

	void Renderer::SetMainViewport(Viewport& viewport)
	{
		s_RendererData.MainViewport = &viewport;
	}

	void Renderer::SetCurrentViewport(Viewport& viewport)
	{
		s_RendererData.CurrentViewport = &viewport;
	}

	void Renderer::BeginFrame()
	{
		s_RendererData.RenderGraphRebuildIsRequired = false;
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

	void Renderer::BeginScene(Viewport& viewport)
	{
	}

	void Renderer::Flush()
	{
		FLARE_PROFILE_FUNCTION();
	}

	void Renderer::EndScene()
	{
		FLARE_PROFILE_FUNCTION();

		FLARE_CORE_ASSERT(s_RendererData.Submition);
		s_RendererData.Submition = nullptr;
	}

	void Renderer::DrawMesh(const Ref<Mesh>& mesh, uint32_t subMesh, const Ref<Material>& material, const glm::mat4& transform, MeshRenderFlags flags)
	{
		s_RendererData.Submition->OpaqueGeometrySubmitions.Submit(mesh, subMesh, material, Math::Compact3DTransform(transform), flags);
	}

	void Renderer::SubmitDecal(const Ref<const Material>& material, const glm::mat4& transform)
	{
		if (material == nullptr || material->GetShader() == nullptr)
			return;

		Math::Compact3DTransform compactTransform(transform);

		auto& decal = s_RendererData.Submition->DecalSubmitions.emplace_back();
		decal.Material = material;
		decal.Transform = Math::Compact3DTransform(transform);
	}

	RendererSubmitionQueue& Renderer::GetOpaqueSubmitionQueue()
	{
		FLARE_CORE_ASSERT(s_RendererData.Submition);
		return s_RendererData.Submition->OpaqueGeometrySubmitions;
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
		s_RendererData.RenderGraphRebuildIsRequired |= settings.Quality != s_RendererData.ShadowMappingSettings.Quality;
		s_RendererData.RenderGraphRebuildIsRequired |= settings.Enabled != s_RendererData.ShadowMappingSettings.Enabled;

		s_RendererData.ShadowMappingSettings = settings;
	}

	bool Renderer::RequiresRenderGraphRebuild()
	{
		return s_RendererData.RenderGraphRebuildIsRequired;
	}

	Ref<const DescriptorSetLayout> Renderer::GetDecalsDescriptorSetLayout()
	{
		return s_RendererData.DecalsDescriptorSetPool->GetLayout();
	}

	static void SetupPrimaryDescriptorSet(Ref<DescriptorSet> set, Ref<Sampler> comparisonSampler)
	{
		for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
		{
			set->WriteImage(s_RendererData.DummyDepthTexture, (uint32_t)(4 + i));
		}

		for (size_t i = 0; i < ShadowSettings::MaxCascades; i++)
		{
			set->WriteImage(s_RendererData.DummyDepthTexture, comparisonSampler, (uint32_t)(8 + i));
		}

		set->FlushWrites();
	}

	static Ref<ShadowPass> ConfigureShadowPass(Viewport& viewport, std::array<RenderGraphTextureId, ShadowSettings::MaxCascades>& cascadeTextures)
	{
		FLARE_PROFILE_FUNCTION();

		RenderGraphPassSpecifications shadowPassSpec{};
		shadowPassSpec.SetDebugName("ShadowPass");

		Ref<ShadowPass> shadowPass = CreateRef<ShadowPass>();

		viewport.Graph.AddPass(shadowPassSpec, shadowPass);

		if (!viewport.IsShadowMappingEnabled())
		{
			return shadowPass;
		}

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
			Ref<Texture> cascadeTexture = Texture::Create(cascadeSpec);
			cascadeTexture->SetDebugName(fmt::format("CascadeTexture.{}", cascadeIndex));

			cascadeTextures[cascadeIndex] = viewport.Graph.GetResourceManager().RegisterExistingTexture(cascadeTexture);

			RenderGraphPassSpecifications cascadePassSpec{};
			cascadePassSpec.SetDebugName(fmt::format("ShadowCascadePass{}", cascadeIndex));
			cascadePassSpec.AddOutput(cascadeTextures[cascadeIndex], 0, 1.0f);

			Ref<ShadowCascadePass> cascadePass = CreateRef<ShadowCascadePass>(
				s_RendererData.Statistics,
				shadowPass->GetCascadeData((size_t)cascadeIndex),
				shadowPass->GetFilteredTransforms(),
				shadowPass->GetVisibleSubMeshIndices());

			viewport.Graph.AddPass(cascadePassSpec, cascadePass);
		}

		return shadowPass;
	}

	void Renderer::ConfigurePasses(Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();
		std::array<RenderGraphTextureId, ShadowSettings::MaxCascades> cascadeTextures = { RenderGraphTextureId() };
		Ref<ShadowPass> shadowPass = ConfigureShadowPass(viewport, cascadeTextures);

		SetupPrimaryDescriptorSet(viewport.GlobalResources.GlobalDescriptorSet, shadowPass->GetCompareSampler());
		SetupPrimaryDescriptorSet(viewport.GlobalResources.GlobalDescriptorSetWithoutShadows, shadowPass->GetCompareSampler());

		RenderGraphPassSpecifications geometryPass{};
		geometryPass.SetDebugName("GeometryPass");
		geometryPass.AddOutput(viewport.ColorTextureId, 0);
		geometryPass.AddOutput(viewport.NormalsTextureId, 1);
		geometryPass.AddOutput(viewport.DepthTextureId, 2);

		if (viewport.IsShadowMappingEnabled())
		{
			Ref<DescriptorSet> set = viewport.GlobalResources.GlobalDescriptorSet;
			for (uint32_t i = 0; i < (uint32_t)Renderer::GetShadowSettings().Cascades; i++)
			{
				Ref<Texture> cascadeTexture = viewport.Graph.GetResourceManager().GetTexture(cascadeTextures[i]);
				set->WriteImage(cascadeTexture, 4 + i);
				set->WriteImage(cascadeTexture, shadowPass->GetCompareSampler(), 8 + i);
			}

			set->FlushWrites();

			for (size_t i = 0; i < cascadeTextures.size(); i++)
			{
				if (cascadeTextures[i].GetValue() == UINT32_MAX)
					break;

				geometryPass.AddInput(cascadeTextures[i]);
			}
		}

		viewport.Graph.AddPass(geometryPass, CreateRef<GeometryPass>(s_RendererData.Statistics));

		// Decal pass
		RenderGraphPassSpecifications decalPass{};
		decalPass.AddInput(viewport.DepthTextureId);
		decalPass.AddOutput(viewport.ColorTextureId, 0);

		viewport.Graph.AddPass(decalPass, CreateRef<DecalsPass>(
			s_RendererData.DecalsDescriptorSetPool,
			viewport.DepthTextureId));
	}
}
