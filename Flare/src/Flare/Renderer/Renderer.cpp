#include "Renderer.h"

#include "Flare/Renderer/RendererPrimitives.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Math/Transform.h"
#include "Flare/Math/SIMD.h"

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
		Viewport* MainViewport = nullptr;
		Viewport* CurrentViewport = nullptr;

		bool RenderGraphRebuildIsRequired = false;

		Ref<UniformBuffer> CameraBuffer = nullptr;
		Ref<UniformBuffer> LightBuffer = nullptr;

		Ref<Texture> WhiteTexture = nullptr;
		Ref<Texture> DefaultNormalMap = nullptr;

		Ref<Material> ErrorMaterial = nullptr;
		Ref<Material> DepthOnlyMeshMaterial = nullptr;
		
		RendererStatistics Statistics;

		RendererSubmitionQueue OpaqueQueue;
		std::vector<DecalSubmitionData> Decals;

		// Shadows
		ShadowSettings ShadowMappingSettings;

		// Lighting
		std::vector<PointLightData> PointLights;
		Ref<ShaderStorageBuffer> PointLightsShaderBuffer = nullptr;
		std::vector<SpotLightData> SpotLights;
		Ref<ShaderStorageBuffer> SpotLightsShaderBuffer = nullptr;

		Ref<DescriptorSet> PrimaryDescriptorSet = nullptr;
		Ref<DescriptorSetPool> PrimaryDescriptorPool = nullptr;

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

		s_RendererData.ShadowMappingSettings.Quality = ShadowQuality::High;
		s_RendererData.ShadowMappingSettings.Bias = 0.0f;
		s_RendererData.ShadowMappingSettings.NormalBias = 0.0f;
		s_RendererData.ShadowMappingSettings.LightSize = 0.02f;
		
		s_RendererData.ShadowMappingSettings.FadeDistance = 80.0f;

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
			VkDescriptorSetLayoutBinding bindings[6 + 4 + 4] = {};
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

			// Shadow cascades (with comapare samplers)
			for (uint32_t i = 0; i < 4; i++)
			{
				bindings[i + 10].binding = i + 32;
				bindings[i + 10].descriptorCount = 1;
				bindings[i + 10].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				bindings[i + 10].pImmutableSamplers = nullptr;
				bindings[i + 10].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			}

			s_RendererData.PrimaryDescriptorPool = CreateRef<VulkanDescriptorSetPool>(64, Span(bindings, 6 + 4 + 4));
			s_RendererData.PrimaryDescriptorSet = s_RendererData.PrimaryDescriptorPool->AllocateSet();
			s_RendererData.PrimaryDescriptorSet->SetDebugName("PrimarySet");

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
			
			// Decals descriptor set
			VkDescriptorSetLayoutBinding decalDepthBinding{};
			decalDepthBinding.binding = 0;
			decalDepthBinding.descriptorCount = 1;
			decalDepthBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			decalDepthBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			s_RendererData.DecalsDescriptorSetPool = CreateRef<VulkanDescriptorSetPool>(2, Span(&decalDepthBinding, 1));
		}

		Project::OnProjectOpen.Bind(ReloadShaders);
	}

	void Renderer::Shutdown()
	{
		s_RendererData.PrimaryDescriptorPool->ReleaseSet(s_RendererData.PrimaryDescriptorSet);

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

	void Renderer::Flush()
	{
		FLARE_PROFILE_FUNCTION();

		s_RendererData.Statistics.ObjectsSubmitted += (uint32_t)s_RendererData.OpaqueQueue.GetSize();
		s_RendererData.CurrentViewport->Graph.Execute(GraphicsContext::GetInstance().GetCommandBuffer());
	}

	void Renderer::EndScene()
	{
		s_RendererData.PointLights.clear();
		s_RendererData.SpotLights.clear();
		s_RendererData.Decals.clear();

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

		Math::Compact3DTransform compactTransform(transform);

		auto& decal = s_RendererData.Decals.emplace_back();
		decal.Material = material;
		decal.PackedTransform[0] = glm::vec4(compactTransform.RotationScale[0], compactTransform.Translation.x);
		decal.PackedTransform[1] = glm::vec4(compactTransform.RotationScale[1], compactTransform.Translation.y);
		decal.PackedTransform[2] = glm::vec4(compactTransform.RotationScale[2], compactTransform.Translation.z);
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

	Ref<DescriptorSet> Renderer::GetPrimaryDescriptorSet()
	{
		return s_RendererData.PrimaryDescriptorSet;
	}

	Ref<const DescriptorSetLayout> Renderer::GetPrimaryDescriptorSetLayout()
	{
		return s_RendererData.PrimaryDescriptorPool->GetLayout();
	}

	Ref<const DescriptorSetLayout> Renderer::GetDecalsDescriptorSetLayout()
	{
		return s_RendererData.DecalsDescriptorSetPool->GetLayout();
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
		FLARE_PROFILE_FUNCTION();

		RenderGraphPassSpecifications shadowPassSpec{};
		shadowPassSpec.SetDebugName("ShadowPass");

		Ref<ShadowPass> shadowPass = CreateRef<ShadowPass>(s_RendererData.OpaqueQueue);

		viewport.Graph.AddPass(shadowPassSpec, shadowPass);

		if (!viewport.IsShadowMappingEnabled())
		{
			return shadowPass;
		}

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

			for (uint32_t i = 0; i < ShadowSettings::MaxCascades; i++)
			{
				set->WriteImage(s_RendererData.WhiteTexture, 28 + i);
			}

			for (uint32_t i = 0; i < ShadowSettings::MaxCascades; i++)
			{
				set->WriteImage(s_RendererData.WhiteTexture, shadowPass->GetCompareSampler(), 32 + i);
			}

			set->FlushWrites();
			set->SetDebugName(fmt::format("Cascade{}.PrimaryDescriptorSet", cascadeIndex));

			RenderGraphPassSpecifications cascadePassSpec{};
			cascadePassSpec.SetDebugName(fmt::format("ShadowCascadePass{}", cascadeIndex));
			cascadePassSpec.AddOutput(cascadeTextures[cascadeIndex], 0, 1.0f);

			Ref<ShadowCascadePass> cascadePass = CreateRef<ShadowCascadePass>(
				s_RendererData.OpaqueQueue,
				s_RendererData.Statistics,
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
		FLARE_PROFILE_FUNCTION();
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

		if (viewport.IsShadowMappingEnabled())
		{
			for (size_t i = 0; i < cascadeTextures.size(); i++)
			{
				if (cascadeTextures[i] == nullptr)
					break;

				geometryPass.AddInput(cascadeTextures[i]);
			}

			// Fill first cascades with textures from shadow casacde passes,
			// the rest of cascades were filled with white textures when setting up the descriptor set
			for (uint32_t i = 0; i < (uint32_t)s_RendererData.ShadowMappingSettings.Cascades; i++)
			{
				primarySet->WriteImage(cascadeTextures[i], 28 + i);
				primarySet->WriteImage(cascadeTextures[i], shadowPass->GetCompareSampler(), 32 + i);
			}
		}

		primarySet->FlushWrites();

		viewport.Graph.AddPass(geometryPass, CreateRef<GeometryPass>(
			s_RendererData.OpaqueQueue,
			s_RendererData.Statistics,
			primarySet,
			primarySetWithoutShadows,
			s_RendererData.PrimaryDescriptorPool));

		// Decal pass
		RenderGraphPassSpecifications decalPass{};
		decalPass.AddInput(viewport.DepthTexture);
		decalPass.AddOutput(viewport.ColorTexture, 0);

		Ref<DescriptorSet> decalPrimarySet = s_RendererData.PrimaryDescriptorPool->AllocateSet();
		decalPrimarySet->SetDebugName("DecalPrimarySet");
		SetupPrimaryDescriptorSet(decalPrimarySet, shadowDataBuffer);

		viewport.Graph.AddPass(decalPass, CreateRef<DecalsPass>(
			s_RendererData.Decals,
			decalPrimarySet,
			s_RendererData.PrimaryDescriptorPool,
			s_RendererData.DecalsDescriptorSetPool,
			viewport.DepthTexture));
	}
}
