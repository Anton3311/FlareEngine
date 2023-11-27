#include "Scene.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/Renderer/DebugRenderer.h"

#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Math/Math.h"
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

	struct ShadowMappingParams
	{
		glm::vec3 ViewPosition;
		glm::vec3 CameraFrustumCenter;
		float BoundingSphereRadius;
	};

	static void CalculateShadowParams(ShadowMappingParams& params, glm::vec3 lightDirection, const Viewport& viewport)
	{
		const CameraData& camera = viewport.FrameData.Camera;
		const ShadowSettings& shadowSettings = Renderer::GetShadowSettings();

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

		Math::Plane farPlane = Math::Plane::TroughPoint(camera.Position + camera.ViewDirection * shadowSettings.MaxDistance, camera.ViewDirection);
		for (size_t i = 0; i < frustumCorners.size() / 2; i++)
		{
			Math::Ray ray;
			ray.Origin = frustumCorners[i];
			ray.Direction = frustumCorners[i + 4] - frustumCorners[i];

			frustumCorners[i + 4] = glm::vec4(ray.Origin + ray.Direction * Math::IntersectPlane(farPlane, ray), 0.0f);
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

	void Scene::OnBeforeRender(Viewport& viewport)
	{
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
					TransformComponent& transform = transforms[entity];

					float halfSize = camera.Size / 2;

					viewport.FrameData.Camera.ViewDirection = transform.TransformDirection(glm::vec3(0.0f, 0.0f, -1.0f));

					viewport.FrameData.Camera.Near = camera.Near;
					viewport.FrameData.Camera.Far = camera.Far;
					viewport.FrameData.Camera.View = glm::inverse(transforms[entity].GetTransformationMatrix());

					viewport.FrameData.Camera.FOV = cameras[entity].FOV;

					if (camera.Projection == CameraComponent::ProjectionType::Orthographic)
						viewport.FrameData.Camera.Projection = glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, camera.Near, camera.Far);
					else
						viewport.FrameData.Camera.Projection = glm::perspective<float>(glm::radians(camera.FOV), aspectRation, camera.Near, camera.Far);

					viewport.FrameData.Camera.Position = transforms[entity].Position;
					viewport.FrameData.Camera.CalculateViewProjection();
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

				ShadowMappingParams params;
				CalculateShadowParams(params, direction, viewport);

				glm::mat4 view = glm::lookAt(params.ViewPosition, params.CameraFrustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

				LightData& light = viewport.FrameData.Light;
				light.Color = lights[entity].Color;
				light.Intensity = lights[entity].Intensity;
				light.Direction = -direction;
				light.Near = 0.1f;
				light.Far = params.BoundingSphereRadius * 2.0f;

				CameraData& lightView = viewport.FrameData.LightView;
				lightView.Position = transform.Position;
				lightView.View = view;
				lightView.Projection = glm::ortho(
					-params.BoundingSphereRadius,
					params.BoundingSphereRadius,
					-params.BoundingSphereRadius,
					params.BoundingSphereRadius,
					light.Near, light.Far);

				lightView.CalculateViewProjection();

				light.LightProjection = lightView.ViewProjection;
				hasLight = true;

				break;
			}

			if (hasLight)
				break;
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