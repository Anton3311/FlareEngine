#include "Scene.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/DebugRenderer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Math/Math.h"
#include "Flare/Scene/Components.h"
#include "Flare/Scene/Hierarchy.h"
#include "Flare/Scene/Transform.h"
#include "Flare/Input/InputManager.h"

#include "Flare/Scripting/ScriptingEngine.h"

namespace Flare
{
	Ref<Scene> s_Active = nullptr;

	FLARE_SERIALIZABLE_IMPL(Scene);
	FLARE_IMPL_ASSET(Scene);

	Scene::Scene(ECSContext& context)
		: Asset(AssetType::Scene), m_World(context)
	{
		m_PostProcessingManager.SSAOPass = CreateRef<SSAO>();
		m_PostProcessingManager.VignettePass = CreateRef<Vignette>();
		m_PostProcessingManager.Atmosphere = CreateRef<AtmospherePass>();
		m_PostProcessingManager.ToneMappingPass = CreateRef<ToneMapping>();

		m_World.MakeCurrent();
		Initialize();
	}

	Scene::~Scene()
	{
	}

	void Scene::Initialize()
	{
		ScriptingEngine::SetCurrentECSWorld(m_World);

		SystemsManager& systemsManager = m_World.GetSystemsManager();
		systemsManager.CreateGroup("Debug Rendering");

		m_2DRenderingGroup = systemsManager.CreateGroup("2D Rendering");
		m_ScriptingUpdateGroup = systemsManager.CreateGroup("Scripting Update");
		m_LateUpdateGroup = systemsManager.CreateGroup("Late Update");
		m_OnRuntimeStartGroup = systemsManager.CreateGroup("On Runtime Start");
		m_OnRuntimeEndGroup = systemsManager.CreateGroup("On Runtime End");

		m_OnFrameStart = systemsManager.CreateGroup("On Frame End");
		m_OnFrameEnd = systemsManager.CreateGroup("On Frame End");

		m_CameraDataUpdateQuery = m_World.NewQuery().All().With<TransformComponent, CameraComponent>().Build();
		m_DirectionalLightQuery = m_World.NewQuery().All().With<TransformComponent, DirectionalLight>().Build();
		m_EnvironmentQuery = m_World.NewQuery().All().With<Environment>().Build();
		m_PointLightsQuery = m_World.NewQuery().All().With<TransformComponent, PointLight>().Build();
		m_SpotLightsQuery = m_World.NewQuery().All().With<TransformComponent, SpotLight>().Build();

		systemsManager.RegisterSystem("Sprites Renderer", m_2DRenderingGroup, new SpritesRendererSystem());
		systemsManager.RegisterSystem("Meshes Renderer", m_2DRenderingGroup, new MeshesRendererSystem());
	}

	void Scene::InitializeRuntime()
	{
		ScriptingEngine::RegisterSystems();
		m_World.GetSystemsManager().RebuildExecutionGraphs();
	}

	void Scene::InitializePostProcessing()
	{
		Renderer::AddRenderPass(m_PostProcessingManager.SSAOPass);
		Renderer::AddRenderPass(m_PostProcessingManager.Atmosphere);
		Renderer::AddRenderPass(m_PostProcessingManager.VignettePass);
		Renderer::AddRenderPass(m_PostProcessingManager.ToneMappingPass);
	}

	void Scene::UninitializePostProcessing()
	{
		Renderer::RemoveRenderPass(m_PostProcessingManager.VignettePass);
		Renderer::RemoveRenderPass(m_PostProcessingManager.SSAOPass);
		Renderer::RemoveRenderPass(m_PostProcessingManager.ToneMappingPass);
		Renderer::RemoveRenderPass(m_PostProcessingManager.Atmosphere);
	}

	void Scene::OnRuntimeStart()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_OnRuntimeStartGroup);
	}

	void Scene::OnRuntimeEnd()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_OnRuntimeEndGroup);
	}

	void Scene::OnBeforeRender(Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		if (!viewport.FrameData.IsEditorCamera)
		{
			float aspectRation = viewport.GetAspectRatio();

			for (EntityView entityView : m_CameraDataUpdateQuery)
			{
				auto cameras = entityView.View<const CameraComponent>();
				auto transforms = entityView.View<const TransformComponent>();

				for (EntityViewElement entity : entityView)
				{
					const CameraComponent& camera = cameras[entity];
					const TransformComponent& transform = transforms[entity];

					float halfSize = camera.Size / 2;

					viewport.FrameData.Camera.ViewDirection = transform.TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));
					viewport.FrameData.Camera.Position = transforms[entity].Position;

					viewport.FrameData.Camera.Near = camera.Near;
					viewport.FrameData.Camera.Far = camera.Far;

					viewport.FrameData.Camera.FOV = cameras[entity].FOV;

					if (RendererAPI::GetAPI() == RendererAPI::API::Vulkan)
					{
						if (camera.Projection == CameraComponent::ProjectionType::Orthographic)
						{
							viewport.FrameData.Camera.SetViewAndProjection(
								glm::orthoRH_ZO(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, camera.Near, camera.Far),
								glm::inverse(transforms[entity].GetTransformationMatrix()));
						}
						else
						{
							viewport.FrameData.Camera.SetViewAndProjection(
								glm::perspectiveRH_ZO<float>(glm::radians(camera.FOV), aspectRation, camera.Near, camera.Far),
								glm::inverse(transforms[entity].GetTransformationMatrix()));
						}
					}
					else
					{
						if (camera.Projection == CameraComponent::ProjectionType::Orthographic)
						{
							viewport.FrameData.Camera.SetViewAndProjection(
								glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, camera.Near, camera.Far),
								glm::inverse(transforms[entity].GetTransformationMatrix()));
						}
						else
						{
							viewport.FrameData.Camera.SetViewAndProjection(
								glm::perspective<float>(glm::radians(camera.FOV), aspectRation, camera.Near, camera.Far),
								glm::inverse(transforms[entity].GetTransformationMatrix()));
						}
					}
				}
			}
		}

		bool hasLight = false;
		for (EntityView entityView : m_DirectionalLightQuery)
		{
			auto transforms = entityView.View<TransformComponent>();
			auto lights = entityView.View<DirectionalLight>();

			for (EntityViewElement entity : entityView)
			{
				TransformComponent& transform = transforms[entity];
				glm::vec3 direction = transform.TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));
				glm::vec3 right = transform.TransformDirection(glm::vec3(1.0f, 0.0f, 0.0f));

				viewport.FrameData.LightBasis.Right = right;
				viewport.FrameData.LightBasis.Forward = direction;
				viewport.FrameData.LightBasis.Up = glm::cross(right, direction);

				LightData& light = viewport.FrameData.Light;
				light.Color = lights[entity].Color;
				light.Intensity = lights[entity].Intensity;
				light.Direction = direction;
				light.Near = 0.1f;

				for (size_t i = 0; i < 4; i++)
				{
					RenderView& lightView = viewport.FrameData.LightView[i];
					lightView.Position = transform.Position;
				}

				hasLight = true;
				break;
			}

			if (hasLight)
				break;
		}

		bool hasEnviroment = false;
		for (EntityView view : m_EnvironmentQuery)
		{
			auto environments = view.View<const Environment>();

			for (EntityViewElement entity : view)
			{
				viewport.FrameData.Light.EnvironmentLight = glm::vec4(
					environments[entity].EnvironmentColor,
					environments[entity].EnvironmentColorIntensity
				);

				Renderer::GetShadowSettings() = environments[entity].ShadowSettings;

				hasEnviroment = true;
				break;
			}

			if (hasEnviroment)
				break;
		}

		for (EntityView view : m_PointLightsQuery)
		{
			auto transforms = view.View<const TransformComponent>();
			auto lights = view.View<const PointLight>();

			for (EntityViewElement entity : view)
			{
				Renderer::SubmitPointLight(PointLightData{
					transforms[entity].Position,
					glm::vec4(lights[entity].Color, lights[entity].Intensity)
				});
			}
		}

		for (EntityView view : m_SpotLightsQuery)
		{
			auto transforms = view.View<const TransformComponent>();
			auto lights = view.View<const SpotLight>();
			auto globalTransform = view.ViewOptional<const GlobalTransform>();

			for (EntityViewElement entity : view)
			{
				if (lights[entity].OuterAngle - lights[entity].InnerAngle <= 0.0f)
					continue;

				glm::vec3 position = transforms[entity].Position;
				glm::vec3 direction = glm::vec3(0.0f);

				if (globalTransform.HasComponent())
				{
					position = globalTransform[entity].value()->Position;
					direction = globalTransform[entity].value()->TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));
				}
				else
				{
					position = transforms[entity].Position;
					direction = transforms[entity].TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));
				}

				Renderer::SubmitSpotLight(SpotLightData(
					position,
					direction,					
					lights[entity].InnerAngle,
					lights[entity].OuterAngle,
					glm::vec4(lights[entity].Color, lights[entity].Intensity)
				));
			}
		}
	}

	void Scene::OnRender(const Viewport& viewport)
	{
		FLARE_PROFILE_FUNCTION();

		Renderer2D::Begin();

		m_World.GetSystemsManager().ExecuteGroup(m_2DRenderingGroup);

		Renderer2D::End();
		Renderer::Flush();

		Renderer::ExecutePostProcessingPasses();
	}

	void Scene::OnUpdateRuntime()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_OnFrameStart);
		m_World.GetSystemsManager().ExecuteGroup(m_ScriptingUpdateGroup);
		m_World.GetSystemsManager().ExecuteGroup(m_LateUpdateGroup);
		m_World.GetSystemsManager().ExecuteGroup(m_OnFrameEnd);

		// TODO: should probably move out of here, because OnUpdateRuntime is called
		//       only if the game isn't paused, however clearing deleted entites should
		//       be done regardless of the pause state
		m_World.Entities.ClearQueuedForDeletion();
		m_World.Entities.ClearCreatedEntitiesQueryResult();
	}

	void Scene::OnUpdateEditor()
	{
		m_World.GetSystemsManager().ExecuteSystem<TransformPropagationSystem>();
		m_World.Entities.ClearQueuedForDeletion();
		m_World.Entities.ClearCreatedEntitiesQueryResult();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
	}

	World& Scene::GetECSWorld()
	{
		return m_World;
	}

	Ref<Scene> Scene::GetActive()
	{
		return s_Active;
	}

	void Scene::SetActive(const Ref<Scene>& scene)
	{
		s_Active = scene;
	}
}