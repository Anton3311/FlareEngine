#include "SceneRenderer.h"

namespace Flare
{
	SpritesRendererSystem::SpritesRendererSystem()
	{
		m_SpritesQuery = World::GetCurrent().CreateQuery<With<TransformComponent>, With<SpriteComponent>>();
	}

	void SpritesRendererSystem::OnUpdate(SystemExecutionContext& context)
	{
		for (EntityView view : m_SpritesQuery)
		{
			ComponentView<TransformComponent> transforms = view.View<TransformComponent>();
			ComponentView<SpriteComponent> sprites = view.View<SpriteComponent>();

			for (EntityViewIterator entityIterator = view.begin(); entityIterator != view.end(); ++entityIterator)
			{
				TransformComponent& transform = transforms[*entityIterator];
				SpriteComponent& sprite = sprites[*entityIterator];

				auto entity = view.GetEntity(entityIterator.GetEntityIndex());
				if (!entity)
					continue;

				Renderer2D::DrawQuad(transform.GetTransformationMatrix(), sprite.Color,
					sprite.Texture == NULL_ASSET_HANDLE
					? nullptr
					: AssetManager::GetAsset<Texture>(sprite.Texture),
					sprite.TextureTiling, (int32_t)entity.value().GetIndex(),
					sprite.Flags);
			}
		}
	}
}