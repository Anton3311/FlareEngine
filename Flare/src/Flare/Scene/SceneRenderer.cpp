#include "SceneRenderer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Scene/Components.h"
#include "Flare/Scene/Scene.h"
#include "Flare/Scene/Transform.h"

#include "Flare/AssetManager/AssetManager.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/Renderer/GraphicsContext.h"
#include "Flare/Renderer/Renderer.h"

#include "Flare/DebugRenderer/DebugRenderer.h"

#include <algorithm>

namespace Flare
{
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
		SystemsManager& systemsManager = world.GetSystemsManager();

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
				for (auto entity : chunk)
				{
					PointLightSubmition& submition = submitions->emplace_back();
					submition.Color = lights[entity].Color;
					submition.Intensity = lights[entity].Intensity;
					submition.Position = transforms[entity].Position;
				}
			});

		m_SpotLightsQuery.ForEachChunk([submitions = &m_SceneSubmition.SpotLights](QueryChunk chunk,
			ComponentView<const TransformComponent> transforms,
			ComponentView<const SpotLight> lights)
			{
				for (auto entity : chunk)
				{
					if (lights[entity].OuterAngle - lights[entity].InnerAngle <= 0.0f)
						continue;

					glm::vec3 position = transforms[entity].Position;
					glm::vec3 direction = transforms[entity].TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));

					SpotLightSubmition& submition = submitions->emplace_back();
					submition.Color = lights[entity].Color;
					submition.Intensity = lights[entity].Intensity;
					submition.Direction = direction;
					submition.Position = position;
					submition.InnerAngleCos = glm::cos(glm::radians(lights[entity].InnerAngle));
					submition.OuterAngleCos = glm::cos(glm::radians(lights[entity].OuterAngle));
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

	void SceneRenderer::RenderViewport(Viewport& viewport, const RenderView* view)
	{
		FLARE_PROFILE_FUNCTION();

		Renderer::SetCurrentViewport(viewport);

		RenderView sceneCameraView{};
		if (view != nullptr)
		{
			sceneCameraView = *view;
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

		sceneCameraView.ViewportSize = viewport.GetSize();

		PrepareViewportForRendering(viewport, sceneCameraView);

		viewport.GetRenderGraph()->Execute(GraphicsContext::GetInstance().GetCommandBuffer(), m_SceneSubmition, sceneCameraView);
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
		m_SpotLightsQuery = world.NewQuery().All().With<TransformComponent, SpotLight>().Build();
	}

	void SceneRenderer::PrepareViewportForRendering(Viewport& viewport, const RenderView& view)
	{
		FLARE_PROFILE_FUNCTION();

		viewport.PrepareViewport();

		Ref<CommandBuffer> commandBuffer = GraphicsContext::GetInstance().GetCommandBuffer();

		LightData lightData{};
		lightData.Color = m_SceneSubmition.DirectionalLight.Color;
		lightData.Intensity = m_SceneSubmition.DirectionalLight.Intensity;
		lightData.Direction = m_SceneSubmition.DirectionalLight.Direction;

		lightData.Near = 0.1f;

		lightData.EnvironmentLight = glm::vec4(m_SceneSubmition.Environment.EnvironmentColor, m_SceneSubmition.Environment.EnvironmentColorIntensity);

		lightData.PointLightsCount = (uint32_t)m_SceneSubmition.PointLights.size();
		lightData.SpotLightsCount = (uint32_t)m_SceneSubmition.SpotLights.size();

		const ViewportFrameResources& viewportFrameResources = viewport.GetFrameResources();

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

			MemorySpan spotLightsData = MemorySpan::FromVector(m_SceneSubmition.SpotLights);

			if (spotLightsData.GetSize() > viewportFrameResources.SpotLightsBuffer->GetSize())
			{
				viewportFrameResources.SpotLightsBuffer->Resize(spotLightsData.GetSize());
				updateViewportDescriptorSets = true;
			}

			viewportFrameResources.SpotLightsBuffer->SetData(spotLightsData, 0, commandBuffer);
		}

		viewport.UpdateGlobalDescriptorSets();
	}


	//
	// Renderer Submition Systems
	//
	
	FLARE_IMPL_SYSTEM(SpriteRendererSystem);
	void SpriteRendererSystem::OnConfig(World& world, SystemConfig& config)
	{
		FLARE_PROFILE_FUNCTION();
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("Rendering");
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
				for (auto entity : chunk)
				{
					const auto& sprite = sprites[entity];
					const auto& transform = transforms[entity];

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
				for (auto entity : chunk)
				{
					glm::mat4 transform = transforms[entity].GetTransformationMatrix();
					const TextComponent& text = textComponents[entity];

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
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("Rendering");
		FLARE_CORE_ASSERT(groupId);
		config.Group = *groupId;

		m_Query = world.NewQuery().All().With<TransformComponent, MeshRenderer>().Build();
	}

	void MeshRendererSystem::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		RendererSubmitionQueue& submitionQueue = Renderer::GetOpaqueSubmitionQueue();
		m_Query.ForEachChunk([&](QueryChunk chunk, ComponentView<const TransformComponent> transforms, ComponentView<MeshRenderer> meshRenderers)
			{
				for (auto entity : chunk)
				{
					MeshRenderer& meshRenderer = meshRenderers[entity];
					if (meshRenderer.Mesh == nullptr)
						continue;

					submitionQueue.Submit(meshRenderer.Mesh,
						Span<Ref<Material>>::FromVector(meshRenderer.Materials),
						Math::Compact3DTransform(transforms[entity].GetTransformationMatrix()),
						meshRenderer.Flags);
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
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("Rendering");
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
				for (auto entity : chunk)
				{
					if (decals[entity].Material == nullptr || decals[entity].Material->GetShader() == nullptr)
						continue;

					auto& decal = sceneSubmition.DecalSubmitions.emplace_back();
					decal.Material = decals[entity].Material;
					decal.Transform = Math::Compact3DTransform(transforms[entity].GetTransformationMatrix());
				}
			});
	}
}