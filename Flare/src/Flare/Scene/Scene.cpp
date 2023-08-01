#include "Scene.h"

#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Scene/Components.h"

#include "Flare/Scripting/ScriptingEngine.h"

namespace Flare
{
	Ref<Scene> Scene::s_Active = nullptr;

	Scene::Scene(bool registerComponents)
		: Asset(AssetType::Scene)
	{
		m_QuadShader = Shader::Create("assets/Shaders/QuadShader.glsl");
		if (registerComponents)
		{
			m_World.RegisterComponent<TransformComponent>();
			m_World.RegisterComponent<CameraComponent>();
			m_World.RegisterComponent<SpriteComponent>();

			Initialize();
		}
	}

	Scene::~Scene()
	{
	}

	void Scene::CopyFrom(const Ref<Scene>& scene)
	{
		for (const ComponentInfo& component : scene->m_World.GetRegistry().GetRegisteredComponents())
			m_World.GetRegistry().RegisterComponent(component.Name, component.Size, component.Deleter);

		for (Entity entity : scene->m_World.GetRegistry())
		{
			Entity newEntity = m_World.GetRegistry().CreateEntity(ComponentSet(scene->m_World.GetEntityComponents(entity)));

			std::optional<size_t> entitySize = scene->m_World.GetRegistry().GetEntityDataSize(entity);

			std::optional<uint8_t*> newEntityData = m_World.GetRegistry().GetEntityData(newEntity);
			std::optional<const uint8_t*> entityData = scene->m_World.GetRegistry().GetEntityData(entity);

			if (entitySize.has_value() && newEntityData.has_value() && entityData.has_value())
				std::memcpy(newEntityData.value(), entityData.value(), entitySize.value());
		}
	}

	void Scene::Initialize()
	{
		ScriptingEngine::SetCurrentECSWorld(m_World);
		ScriptingEngine::RegisterComponents();
		ScriptingEngine::CreateSystems();

		SystemsManager& systemsManager = m_World.GetSystemsManager();
		m_2DRenderingGroup = systemsManager.CreateGroup("2D Rendering");
		m_ScriptingUpdateGroup = systemsManager.CreateGroup("Scripting Update");
		m_OnRuntimeStartGroup = systemsManager.CreateGroup("On Runtime Start");
		m_OnRuntimeEndGroup = systemsManager.CreateGroup("On Runtime End");

		m_CameraDataUpdateQuery = m_World.CreateQuery<With<TransformComponent>, With<CameraComponent>>();

		systemsManager.RegisterSystem("Sprites Renderer", m_2DRenderingGroup, SpritesRendererSystem::Setup(m_World), 
			nullptr, SpritesRendererSystem::OnUpdate, nullptr);
	}

	void Scene::InitializeRuntime()
	{
		ScriptingEngine::RegisterSystems();
	}

	void Scene::OnRuntimeStart()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_OnRuntimeStartGroup);
	}

	void Scene::OnRuntimeEnd()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_OnRuntimeEndGroup);
	}

	void Scene::OnBeforeRender(RenderData& renderData)
	{
		if (!renderData.IsEditorCamera)
		{
			for (EntityView entityView : m_CameraDataUpdateQuery)
			{
				ComponentView<CameraComponent> cameras = entityView.View<CameraComponent>();
				ComponentView<TransformComponent> transforms = entityView.View<TransformComponent>();

				for (EntityViewElement entity : entityView)
				{
					CameraComponent& camera = cameras[entity];

					float halfSize = camera.Size / 2;
					float aspectRation = (float)renderData.ViewportSize.x / (float)renderData.ViewportSize.y;

					renderData.Camera.ViewMatrix = glm::inverse(transforms[entity].GetTransformationMatrix());

					if (camera.Projection == CameraComponent::ProjectionType::Orthographic)
						renderData.Camera.ProjectionMatrix = glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, camera.Near, camera.Far);
					else
						renderData.Camera.ProjectionMatrix = glm::perspective<float>(glm::radians(camera.FOV), aspectRation, camera.Near, camera.Far);

					renderData.Camera.CalculateViewProjection();
				}
			}
		}
	}

	void Scene::OnRender(const RenderData& renderData)
	{
		Renderer2D::Begin(m_QuadShader, renderData.Camera.ViewProjectionMatrix);

		m_World.GetSystemsManager().ExecuteGroup(m_2DRenderingGroup);

		Renderer2D::End();
	}

	void Scene::OnUpdateRuntime()
	{
		m_World.GetSystemsManager().ExecuteGroup(m_ScriptingUpdateGroup);
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
	}
}