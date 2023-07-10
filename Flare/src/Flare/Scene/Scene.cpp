#include "Scene.h"

#include "Flare/Renderer2D/Renderer2D.h"
#include "Flare/AssetManager/AssetManager.h"

#include "Flare/Scene/Components.h"

namespace Flare
{
	Scene::Scene()
		: Asset(AssetType::Scene)
	{
		m_QuadShader = Shader::Create("QuadShader.glsl");

		m_World.RegisterComponent<TransformComponent>();
		m_World.RegisterComponent<CameraComponent>();
		m_World.RegisterComponent<SpriteComponent>();
		
		m_CameraDataUpdateQuery = m_World.CreateQuery<TransformComponent, CameraComponent>();
		m_SpritesQuery = m_World.CreateQuery<TransformComponent, SpriteComponent>();
	}

	Scene::~Scene()
	{
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

					glm::mat4 inverseTransform = glm::inverse(transforms[entity].GetTransformationMatrix());

					float halfSize = camera.Size / 2;
					float aspectRation = (float)renderData.ViewportSize.x / (float)renderData.ViewportSize.y;

					if (camera.Projection == CameraComponent::ProjectionType::Orthographic)
						renderData.Camera.Projection = glm::ortho(-halfSize * aspectRation, halfSize * aspectRation, -halfSize, halfSize, camera.Near, camera.Far);
					else
						renderData.Camera.Projection = glm::perspective<float>(glm::radians(camera.FOV), aspectRation, camera.Near, camera.Far);

					renderData.Camera.Projection *= inverseTransform;
				}
			}
		}
	}

	void Scene::OnRender(const RenderData& renderData)
	{
		Renderer2D::Begin(m_QuadShader, renderData.Camera.Projection);

		for (EntityView view : m_SpritesQuery)
		{
			ComponentView<TransformComponent> transforms = view.View<TransformComponent>();
			ComponentView<SpriteComponent> sprites = view.View<SpriteComponent>();

			for (EntityViewElement entity : view)
			{
				TransformComponent& transform = transforms[entity];
				SpriteComponent& sprite = sprites[entity];

				Renderer2D::DrawQuad(transform.GetTransformationMatrix(), sprite.Color,
					sprite.Texture == NULL_ASSET_HANDLE
						? nullptr
						: AssetManager::GetAsset<Texture>(sprite.Texture),
					sprite.TextureTiling);
			}
		}

		Renderer2D::End();
	}

	void Scene::OnUpdateRuntime()
	{
		m_World.OnUpdate();
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
	}
}