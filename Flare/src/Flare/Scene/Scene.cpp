#include "Scene.h"

#include "Flare/Renderer2D/Renderer2D.h"

#include "Flare/Scene/Components.h"

namespace Flare
{
	Scene::Scene()
	{
		m_CameraData.Projection = glm::mat4(1.0f);
		m_QuadShader = Shader::Create("QuadShader.glsl");

		m_World.RegisterComponent<TransformComponent>();
		m_World.RegisterComponent<CameraComponent>();

		m_World.RegisterSystem(m_World.CreateQuery<CameraComponent>(), [this](EntityView view)
		{
			if (m_ViewportWidth == 0 || m_ViewportHeight == 0)
				return;

			ComponentView<CameraComponent> cameras = view.View<CameraComponent>();
			for (EntityViewElement entity : view)
			{
				CameraComponent& camera = cameras[entity];
				
				float halfSize = camera.Size / 2;
				float aspectRation = (float) m_ViewportWidth / (float) m_ViewportHeight;

				m_CameraData.Projection = glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, camera.Near, camera.Far);
			}
		});

		Entity cameraEntity = m_World.CreateEntity<CameraComponent>();
		CameraComponent& camera = m_World.GetEntityComponent<CameraComponent>(cameraEntity);
		camera.Size = 10.0f;
		camera.Near = 0.1f;
		camera.Far = 10.0f;
	}

	Scene::~Scene()
	{
	}

	void Scene::OnUpdateRuntime()
	{
		// TODO: call between Renderer2D::Begin() and Renderer2D::End()
		// TODO: maually execute system for updating camera data
		m_World.OnUpdate();

		Renderer2D::Begin(m_QuadShader, m_CameraData.Projection);
		Renderer2D::DrawQuad(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(2.0f), glm::vec4(8.0f, 0.3f, 0.2f, 1.0f));
		Renderer2D::End();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;
	}
}