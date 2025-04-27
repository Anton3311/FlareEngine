#include "PCH.h"

#include "SceneRenderer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "FlareECS/System/System.h"
#include "FlareECS/System/SystemsManager.h"
#include "FlareECS/World.h"

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Scene.h"
#include "Flare/Scene/Transform.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/RendererComponents.h"

#include "Flare/DebugRenderer/DebugRenderer.h"

#include <algorithm>

namespace Flare
{
	inline static bool ValidateSpotLight(const SpotLight& light)
	{
		if (light.InnerAngle > light.OuterAngle)
			return false;

		if (light.Intensity <= 0.0f)
			return false;

		return true;
	}

	static void SubmitSpotLight(SceneSubmition& sceneSubmition, const SpotLight& light, const TransformComponent& transform)
	{
		glm::vec3 position = transform.Position;
		glm::vec3 direction = transform.TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));

		SpotLightSubmition& submition = sceneSubmition.SpotLights.emplace_back();
		submition.Color = light.Color;
		submition.Intensity = light.Intensity;
		submition.Direction = direction;
		submition.Position = position;
		submition.InnerAngleCos = glm::cos(glm::clamp(glm::radians(light.InnerAngle), 0.0f, glm::pi<float>()));
		submition.OuterAngleCos = glm::cos(glm::clamp(glm::radians(light.OuterAngle), 0.0f, glm::pi<float>()));
	}

	SceneRenderer::SceneRenderer(Ref<Scene> scene)
		: m_Scene(scene)
	{
		InitializeQueries();
	}

	void SceneRenderer::CollectSceneData()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Scene);

		m_SceneSubmition.Clear();

		World& world = m_Scene->GetECSWorld();
		SystemsManager& systemsManager = m_Scene->GetECSSystemsManager();

		if (std::optional<Entity> cameraEntity = m_CameraQuery.TryGetFirstEntityId())
		{
			const TransformComponent& transform = world.GetEntityComponent<const TransformComponent>(*cameraEntity);
			const CameraComponent& camera = world.GetEntityComponent<const CameraComponent>(*cameraEntity);

			m_SceneSubmition.Camera.NearPlane = camera.Near;
			m_SceneSubmition.Camera.FarPlane = camera.Far;

			if (camera.Projection == CameraComponent::ProjectionType::Perspective)
			{
				m_SceneSubmition.Camera.Projection = CameraSubmition::ProjectionType::Perspective;
				m_SceneSubmition.Camera.FOVAngle = camera.FOV;
				m_SceneSubmition.Camera.Size = 0.0f;
			}
			else
			{
				m_SceneSubmition.Camera.Projection = CameraSubmition::ProjectionType::Orthographic;
				m_SceneSubmition.Camera.FOVAngle = 0.0f;
				m_SceneSubmition.Camera.Size = camera.Size;
			}

			m_SceneSubmition.Camera.Transform = Math::Compact3DTransform(transform.GetTransformationMatrix());
		}

		if (std::optional<Entity> directionalLightEntity = m_DirectionalLightQuery.TryGetFirstEntityId())
		{
			const TransformComponent& transform = world.GetEntityComponent<const TransformComponent>(*directionalLightEntity);
			const DirectionalLight& directionalLight = world.GetEntityComponent<const DirectionalLight>(*directionalLightEntity);

			glm::vec3 direction = transform.TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));
			glm::vec3 right = transform.TransformDirection(glm::vec3(1.0f, 0.0f, 0.0f));

			DirectionalLightSubmition& light = m_SceneSubmition.DirectionalLight;
			light.Color = directionalLight.Color;
			light.Intensity = directionalLight.Intensity;
			light.Direction = direction;
			light.LightBasis.Right = right;
			light.LightBasis.Forward = direction;
			light.LightBasis.Up = glm::cross(right, direction);
		}
		else
		{
			m_SceneSubmition.DirectionalLight = m_DefaultDirectionalLight;
		}

		if (std::optional<Entity> environmentEntity = m_EnvironmentQuery.TryGetFirstEntityId())
		{
			const Environment& environment = world.GetEntityComponent<const Environment>(*environmentEntity);

			m_SceneSubmition.Environment.EnvironmentColor = environment.EnvironmentColor;
			m_SceneSubmition.Environment.EnvironmentColorIntensity = environment.EnvironmentColorIntensity;
		}
		else
		{
			m_SceneSubmition.Environment = m_DefaultEnvironment;
		}

		m_PointLightsQuery.ForEachChunk([submitions = &m_SceneSubmition.PointLights](QueryChunk chunk,
			ComponentView<const TransformComponent> transforms,
			ComponentView<const PointLight> lights)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					PointLightSubmition& submition = submitions->emplace_back();
					submition.Color = lights[entityIndex].Color;
					submition.Intensity = lights[entityIndex].Intensity;
					submition.Position = transforms[entityIndex].Position;
				}
			});

		m_SpotLightsQuery.ForEachChunk([this, &world](QueryChunk chunk,
			ComponentView<const TransformComponent> transforms,
			ComponentView<const SpotLight> lights,
			ArchetypeId archetype)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					if (!ValidateSpotLight(lights[entityIndex]))
						continue;
					SubmitSpotLight(m_SceneSubmition, lights[entityIndex], transforms[entityIndex]);
				}
			});

		m_SpotLightsWithShadowsQuery.ForEachChunk([this, &world](QueryChunk chunk,
			ComponentView<const TransformComponent> transforms,
			ComponentView<const SpotLight> lights,
			ComponentView<const SpotLightShadows> shadows,
			ArchetypeId archetype)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					if (!ValidateSpotLight(lights[entityIndex]))
						continue;

					size_t lightIndex = m_SceneSubmition.SpotLights.size();
					SubmitSpotLight(m_SceneSubmition, lights[entityIndex], transforms[entityIndex]);

					SpotLightShadowsSubmition& submition = m_SceneSubmition.SpotLightShadows.emplace_back();
					submition.LightIndex = static_cast<uint32_t>(lightIndex);
					submition.Near = shadows[entityIndex].Near;
					submition.Far = shadows[entityIndex].Far;
					submition.Bias = shadows[entityIndex].Bias;
					submition.SizePowerOfTwo = shadows[entityIndex].SizePowerOfTwo;
				}
			});

		std::optional<SystemGroupId> renderingGroupId = systemsManager.FindGroup("Rendering");
		std::optional<SystemGroupId> debugRenderingGroupId = systemsManager.FindGroup("Debug Rendering");

		Ref<Scene> previousActiveScene = Scene::GetActive();

		Scene::SetActive(m_Scene);

		Renderer::BeginScene(m_SceneSubmition);
		Renderer2D::BeginScene(m_SceneSubmition);

		if (renderingGroupId)
		{
			FLARE_PROFILE_SCOPE("ExecuteRenderingSystems");
			systemsManager.ExecuteGroup(*renderingGroupId);
		}

		DebugRenderer::BeginScene(m_SceneSubmition);
		if (debugRenderingGroupId)
		{
			FLARE_PROFILE_SCOPE("ExecuteDebugRenderingSystems");
			systemsManager.ExecuteGroup(*debugRenderingGroupId);
		}
		DebugRenderer::EndScene();

		Renderer2D::EndScene();
		Renderer::EndScene();

		Scene::SetActive(previousActiveScene);
	}

	void SceneRenderer::RenderViewport(Entity viewportEntity, const RenderView* viewOverride, const std::function<void(RenderGraph&)>& onRenderGraphBuild)
	{
		FLARE_PROFILE_FUNCTION();

		const World& renderWorld = Renderer::GetRenderWorld();

		const Viewport& viewport = renderWorld.GetEntityComponent<const Viewport>(viewportEntity);
		if (!viewport.IsValid())
			return;

		const ViewportRenderGraph& viewportRenderGraph = renderWorld.GetEntityComponent<const ViewportRenderGraph>(viewportEntity);

		Renderer::PrepareViewport(viewportEntity, onRenderGraphBuild, m_Scene->GetPostProcessingManager());

		RenderView sceneCameraView{};
		if (viewOverride != nullptr)
		{
			sceneCameraView = *viewOverride;
		}
		else
		{
			const CameraSubmition& sceneCamera = m_SceneSubmition.Camera;
			sceneCameraView.Near = sceneCamera.NearPlane;
			sceneCameraView.Far = sceneCamera.FarPlane;
			sceneCameraView.FOV = sceneCamera.FOVAngle;
			sceneCameraView.Position = sceneCamera.Transform.Translation;
			sceneCameraView.ViewDirection = sceneCamera.Transform.TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));

			float viewportAspectRatio = viewport.GetAspectRatio();
			glm::mat4 viewMatrix = glm::inverse(sceneCamera.Transform.ToMatrix4x4());
			if (sceneCamera.Projection == CameraSubmition::ProjectionType::Orthographic)
			{
				float halfSize = sceneCamera.Size / 2.0f;
				sceneCameraView.SetViewAndProjection(
					glm::orthoRH_ZO(
						-halfSize * viewportAspectRatio,
						+halfSize * viewportAspectRatio,
						-halfSize,
						+halfSize,
						sceneCamera.NearPlane,
						sceneCamera.FarPlane),
					viewMatrix);
			}
			else
			{
				sceneCameraView.SetViewAndProjection(
					glm::perspectiveRH_ZO(
						glm::radians(sceneCamera.FOVAngle),
						viewportAspectRatio,
						sceneCamera.NearPlane,
						sceneCamera.FarPlane),
					viewMatrix);
			}
		}

		sceneCameraView.ViewportSize = viewport.Size;

		PrepareViewportForRendering(viewportEntity, sceneCameraView);

		viewportRenderGraph.Graph->Execute(GraphicsContext::GetInstance().GetCommandBuffer(), m_SceneSubmition, sceneCameraView);
	}

	void SceneRenderer::SetDefaultEnvironmentLight(const glm::vec3& color, float intensity)
	{
		m_DefaultEnvironment.EnvironmentColor = color;
		m_DefaultEnvironment.EnvironmentColorIntensity = intensity;
	}

	void SceneRenderer::SetDefaultDirectionalLight(const glm::vec3& direction, const glm::vec3& color, float intensity)
	{
		m_DefaultDirectionalLight.Color = color;
		m_DefaultDirectionalLight.Intensity = intensity;
		m_DefaultDirectionalLight.Direction = direction;

		m_DefaultDirectionalLight.LightBasis.Forward = direction;
		m_DefaultDirectionalLight.LightBasis.Right = glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f));
		m_DefaultDirectionalLight.LightBasis.Up = glm::cross(m_DefaultDirectionalLight.LightBasis.Right, direction);
	}

	void SceneRenderer::InitializeQueries()
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(m_Scene);

		World& world = m_Scene->GetECSWorld();

		m_CameraQuery = world.NewQuery().All().With<TransformComponent, CameraComponent>().Build();
		m_DirectionalLightQuery = world.NewQuery().All().With<TransformComponent, DirectionalLight>().Build();
		m_EnvironmentQuery = world.NewQuery().All().With<Environment>().Build();
		m_PointLightsQuery = world.NewQuery().All().With<TransformComponent, PointLight>().Build();
		m_SpotLightsQuery = world.NewQuery().All()
			.With<TransformComponent, SpotLight>()
			.Without<SpotLightShadows>().Build();

		m_SpotLightsWithShadowsQuery = world.NewQuery().All()
			.With<TransformComponent, SpotLight, SpotLightShadows>()
			.Build();
	}

	void SceneRenderer::PrepareViewportForRendering(Entity viewportEntity, const RenderView& view)
	{
		FLARE_PROFILE_FUNCTION();

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();

		const World& renderWorld = Renderer::GetRenderWorld();

		const AOConfiguration& aoConfiguration = renderWorld.GetEntityComponent<const AOConfiguration>(viewportEntity);
		const ViewportRenderGraph& renderGraph = renderWorld.GetEntityComponent<const ViewportRenderGraph>(viewportEntity);

		LightData lightData{};
		lightData.Color = m_SceneSubmition.DirectionalLight.Color;
		lightData.Intensity = m_SceneSubmition.DirectionalLight.Intensity;
		lightData.Direction = m_SceneSubmition.DirectionalLight.Direction;
		lightData.Near = 0.1f;
		lightData.EnvironmentLight = glm::vec4(m_SceneSubmition.Environment.EnvironmentColor, m_SceneSubmition.Environment.EnvironmentColorIntensity);
		lightData.PointLightsCount = (uint32_t)m_SceneSubmition.PointLights.size();
		lightData.SpotLightsCount = (uint32_t)(m_SceneSubmition.SpotLights.size() - m_SceneSubmition.SpotLightShadows.size());
		lightData.AOEnabled = renderGraph.Graph->GetResourceManager().IsTextureIdValid(aoConfiguration.AOTexture);
		lightData.FirstShadowCastingSpotlight = static_cast<uint32_t>(m_SceneSubmition.SpotLights.size() - m_SceneSubmition.SpotLightShadows.size());
		lightData.ShadowCastingSpotlightCount = static_cast<uint32_t>(m_SceneSubmition.SpotLightShadows.size());

		const ViewportGlobalResources& viewportGlobalResources = renderWorld.GetEntityComponent<const ViewportGlobalResources>(viewportEntity);
		const ViewportFrameResources& viewportFrameResources = viewportGlobalResources.GetCurrentFrameResources();

		{
			FLARE_PROFILE_SCOPE("UpdateLightUniformBuffer");
			viewportFrameResources.LightBuffer->SetData(MemorySpan(&lightData, 1), 0);
		}

		{
			FLARE_PROFILE_SCOPE("UpdateCameraUniformBuffer");
			viewportFrameResources.CameraBuffer->SetData(MemorySpan(&view, 1), 0);
		}

		bool updateViewportDescriptorSets = false;

		{
			FLARE_PROFILE_SCOPE("UploadPointLightsData");

			MemorySpan pointLightsData = MemorySpan::FromVector(m_SceneSubmition.PointLights);

			if (pointLightsData.GetSize() > viewportFrameResources.PointLightsBuffer->GetSize())
			{
				viewportFrameResources.PointLightsBuffer->Resize(pointLightsData.GetSize());
				updateViewportDescriptorSets = true;
			}

			viewportFrameResources.PointLightsBuffer->SetData(pointLightsData, 0, commandBuffer);
		}

		{
			FLARE_PROFILE_SCOPE("UploadSpotLightsData");

			// Put non-shadow casting lights at the front, and shadow casting lights at the back.
			MemorySpan spotLightsData;
			std::vector<SpotLightSubmition> groupedSpotLights;

			if (m_SceneSubmition.SpotLightShadows.size() > 0)
			{
				groupedSpotLights.resize(m_SceneSubmition.SpotLights.size());

				size_t shadowCastingLightIndex = 0;
				size_t frontIndex = 0;
				size_t backIndex = groupedSpotLights.size() - m_SceneSubmition.SpotLightShadows.size();
				for (size_t i = 0; i < m_SceneSubmition.SpotLights.size(); i++)
				{
					if (i == m_SceneSubmition.SpotLightShadows[shadowCastingLightIndex].LightIndex)
					{
						groupedSpotLights[backIndex++] = m_SceneSubmition.SpotLights[i];
						shadowCastingLightIndex++;
					}
					else
					{
						groupedSpotLights[frontIndex++] = m_SceneSubmition.SpotLights[i];
					}
				}

				spotLightsData = MemorySpan::FromVector(groupedSpotLights);
			}
			else
			{
				spotLightsData = MemorySpan::FromVector(m_SceneSubmition.SpotLights);
			}

			if (spotLightsData.GetSize() > viewportFrameResources.SpotLightsBuffer->GetSize())
			{
				viewportFrameResources.SpotLightsBuffer->Resize(spotLightsData.GetSize());
				updateViewportDescriptorSets = true;
			}

			viewportFrameResources.SpotLightsBuffer->SetData(spotLightsData, 0, commandBuffer);
		}

		viewportGlobalResources.SetupGlobalDescriptorSet(viewportFrameResources, viewportFrameResources.GlobalDescriptorSet);
		viewportGlobalResources.SetupGlobalDescriptorSet(viewportFrameResources, viewportFrameResources.GlobalDescriptorSetWithoutShadows);
	}


	//
	// Renderer Submition Systems
	//
	
	FLARE_IMPL_SYSTEM(SpriteRendererSystem);
	void SpriteRendererSystem::OnConfig(World& world, SystemConfig& config)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<uint32_t> groupId = config.SystemsManager.FindGroup("Rendering");
		FLARE_CORE_ASSERT(groupId);
		config.Group = *groupId;

		m_SpritesQuery = world.NewQuery().All().With<TransformComponent, SpriteComponent>().Build();
		m_TextQuery = world.NewQuery().All().With<TransformComponent, TextComponent>().Build();
	}

	void SpriteRendererSystem::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();
		RenderQuads(world, context);
		RenderText(context);
	}

	void SpriteRendererSystem::RenderQuads(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();
		m_SortedEntities.clear();
		m_SortedEntities.reserve(m_SpritesQuery.GetEntitiesCount());

		m_SpritesQuery.ForEachChunk([](QueryChunk chunk, ComponentView<const TransformComponent> transforms, ComponentView<const SpriteComponent> sprites)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					const auto& sprite = sprites[entityIndex];
					const auto& transform = transforms[entityIndex];

					Renderer2D::DrawSprite(sprite.Sprite,
						transform.GetTransformationMatrix(),
						sprite.Color,
						sprite.Tilling,
						sprite.Flags,
						0);
				}
			});

		// TODO: Sorting & materials support
	}

	void SpriteRendererSystem::RenderText(SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		m_TextQuery.ForEachChunk([](QueryChunk chunk, ComponentView<const TransformComponent> transforms, ComponentView<const TextComponent> textComponents)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					glm::mat4 transform = transforms[entityIndex].GetTransformationMatrix();
					const TextComponent& text = textComponents[entityIndex];

					Renderer2D::DrawString(
						text.Text, transform,
						text.Font ? text.Font : Font::GetDefault(),
						text.Color, 0);
				}
			});
	}

	//
	// Mesh Renderer
	//

	FLARE_IMPL_SYSTEM(MeshRendererSystem);
	void MeshRendererSystem::OnConfig(World& world, SystemConfig& config)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<uint32_t> groupId = config.SystemsManager.FindGroup("Rendering");
		FLARE_CORE_ASSERT(groupId);
		config.Group = *groupId;

		m_Query = world.NewQuery().All().With<TransformComponent, MeshRenderer>().Build();
	}

	void MeshRendererSystem::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		GeometryBatcher& batcher = Renderer::GetCurrentSceneSubmition().BatchedGeometry;

		m_Query.ForEachChunk([&](QueryChunk chunk, ComponentView<const TransformComponent> transforms, ComponentView<MeshRenderer> meshRenderers)
			{
				for (uint32_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					MeshRenderer& meshRenderer = meshRenderers[entityIndex];
					if (meshRenderer.Mesh == nullptr)
						continue;

					batcher.SubmitGeometry(meshRenderer.Mesh,
						Span<const Ref<Material>>::FromVector(meshRenderer.Materials),
						transforms[entityIndex].GetTransformationMatrix());
				}
			});
	}

	//
	// Decals Renderer
	//

	FLARE_IMPL_SYSTEM(DecalRendererSystem);
	void DecalRendererSystem::OnConfig(World& world, SystemConfig& config)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<uint32_t> groupId = config.SystemsManager.FindGroup("Rendering");
		FLARE_CORE_ASSERT(groupId);
		config.Group = *groupId;

		m_DecalsQuery = world.NewQuery().All().With<TransformComponent, Decal>().Build();
	}

	void DecalRendererSystem::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		SceneSubmition& sceneSubmition = Renderer::GetCurrentSceneSubmition();

		m_DecalsQuery.ForEachChunk([&](QueryChunk chunk,
			ComponentView<const TransformComponent> transforms,
			ComponentView<const Decal> decals)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					if (decals[entityIndex].Material == nullptr || decals[entityIndex].Material->GetShader() == nullptr)
						continue;

					auto& decal = sceneSubmition.DecalSubmitions.emplace_back();
					decal.Material = decals[entityIndex].Material;
					decal.Transform = Math::Compact3DTransform(transforms[entityIndex].GetTransformationMatrix());
				}
			});
	}
}
