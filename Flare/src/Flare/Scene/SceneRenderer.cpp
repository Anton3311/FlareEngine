#include "SceneRenderer.h"

namespace Flare
{
	Query SpritesRendererSystem::Setup(World& world)
	{
		return world.CreateQuery<With<TransformComponent>, With<SpriteComponent>>();
	}

	void SpritesRendererSystem::OnUpdate(SystemExecutionContext& context)
	{
		for (EntityView view : context.GetQuery())
		{
			ComponentView<TransformComponent> transforms = view.View<TransformComponent>();
			ComponentView<SpriteComponent> sprites = view.View<SpriteComponent>();

			size_t index = 0;
			for (EntityViewElement entity : view)
			{
				TransformComponent& transform = transforms[entity];
				SpriteComponent& sprite = sprites[entity];

				Renderer2D::DrawQuad(transform.GetTransformationMatrix(), sprite.Color,
					sprite.Texture == NULL_ASSET_HANDLE
					? nullptr
					: AssetManager::GetAsset<Texture>(sprite.Texture),
					sprite.TextureTiling, (int32_t)view.GetEntity(index).value_or(Entity()).GetIndex());

				index++;
			}
		}
	}
}