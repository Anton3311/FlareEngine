#include "Scene.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Scene/Components.h"
#include "Flare/Input/InputManager.h"

#include "Flare/Scripting/ScriptingEngine.h"

namespace Flare
{

	Ref<Scene> s_Active = nullptr;

	Scene::Scene(ECSContext& context)
		: Asset(AssetType::Scene), m_World(context)
	{
		m_PostProcessingManager.ToneMappingPass = CreateRef<ToneMapping>();
		m_PostProcessingManager.VignettePass = CreateRef<Vignette>();

		m_World.MakeCurrent();
		Initialize();
	}

	Scene::~Scene()
	{
	}

	void Scene::Initialize()
	{
		ScriptingEngine::SetCurrentECSWorld(m_World);
		m_World.Components.RegisterComponents();

		SystemsManager& systemsManager = m_World.GetSystemsManager();
		systemsManager.CreateGroup("Debug Rendering");

		m_2DRenderingGroup = systemsManager.CreateGroup("2D Rendering");
		m_ScriptingUpdateGroup = systemsManager.CreateGroup("Scripting Update");
		m_OnRuntimeStartGroup = systemsManager.CreateGroup("On Runtime Start");
		m_OnRuntimeEndGroup = systemsManager.CreateGroup("On Runtime End");

		m_CameraDataUpdateQuery = m_World.NewQuery().With<TransformComponent, CameraComponent>().Create();
		m_DirectionalLightQuery = m_World.NewQuery().With<TransformComponent, DirectionalLight>().Create();

		systemsManager.RegisterSystem("Sprites Renderer", m_2DRenderingGroup, new SpritesRendererSystem());
		systemsManager.RegisterSystem("Meshes Renderer", m_2DRenderingGroup, new MeshesRendererSystem());

		Renderer::AddRenderPass(m_PostProcessingManager.ToneMappingPass);
		Renderer::AddRenderPass(m_PostProcessingManager.VignettePass);
	}

	void Scene::InitializeRuntime()
	{
		ScriptingEngine::RegisterSystems();
		m_World.GetSystemsManager().RebuildExecutionGraphs();
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
		bool hasLight = false;
		for (EntityView entityView : m_DirectionalLightQuery)
		{
			auto transforms = entityView.View<TransformComponent>();
			auto lights = entityView.View<DirectionalLight>();

			for (EntityViewElement entity : entityView)
			{
				TransformComponent& transform = transforms[entity];
				glm::vec3 direction = transform.TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));

				LightData& light = viewport.FrameData.Light;
				light.Color = lights[entity].Color;
				light.Intensity = lights[entity].Intensity;
				light.Direction = -direction;
				light.Near = 0.01f;
				light.Far = 100.0f;

				float lightProjectionSize = 50.0f;
				CameraData& lightView = viewport.FrameData.LightView;
				lightView.Position = transform.Position;
				lightView.View = glm::inverse(transforms[entity].GetTransformationMatrix());
				lightView.Projection = glm::ortho(-lightProjectionSize,
					lightProjectionSize,
					-lightProjectionSize,
					lightProjectionSize,
					light.Near,
					light.Far);

				lightView.CalculateViewProjection();

				light.LightProjection = lightView.ViewProjection;
				hasLight = true;
				break;
			}

			if (hasLight)
				break;
		}

		if (!viewport.FrameData.IsEditorCamera)
		{
			float aspectRation = viewport.GetAspectRatio();

			for (EntityView entityView : m_CameraDataUpdateQuery)
			{
				ComponentView<CameraComponent> cameras = entityView.View<CameraComponent>();
				ComponentView<TransformComponent> transforms = entityView.View<TransformComponent>();

				for (EntityViewElement entity : entityView)
				{
					CameraComponent& camera = cameras[entity];

					float halfSize = camera.Size / 2;

					viewport.FrameData.Camera.View = glm::inverse(transforms[entity].GetTransformationMatrix());

					if (camera.Projection == CameraComponent::ProjectionType::Orthographic)
						viewport.FrameData.Camera.Projection = glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, camera.Near, camera.Far);
					else
						viewport.FrameData.Camera.Projection = glm::perspective<float>(glm::radians(camera.FOV), aspectRation, camera.Near, camera.Far);

					viewport.FrameData.Camera.Position = transforms[entity].Position;
					viewport.FrameData.Camera.CalculateViewProjection();
				}
			}
		}
	}

	void Scene::OnRender(const Viewport& viewport)
	{
		Renderer2D::Begin();

		m_World.GetSystemsManager().ExecuteGroup(m_2DRenderingGroup);

		Renderer2D::End();

		Renderer::Flush();
		Renderer::ExecuteRenderPasses();
	}

	void Scene::OnUpdateRuntime()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_ScriptingUpdateGroup);
		m_World.Entities.ClearQueuedForDeletion();
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