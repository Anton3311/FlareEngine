#include "SceneRenderer.h"

#include <algorithm>

namespace Flare
{
	SpritesRendererSystem::SpritesRendererSystem()
	{
		m_SpritesQuery = World::GetCurrent().CreateQuery<With<TransformComponent>, With<SpriteComponent>>();
	}

	void SpritesRendererSystem::OnUpdate(SystemExecutionContext& context)
	{
		m_SortedEntities.clear();
		m_SortedEntities.reserve(m_SpritesQuery.GetEntitiesCount());

		const SpriteLayer defaultSpriteLayer = 0;
		
		for (EntityView view : m_SpritesQuery)
		{
			ComponentView<TransformComponent> transforms = view.View<TransformComponent>();
			ComponentView<SpriteComponent> sprites = view.View<SpriteComponent>();
			auto layers = view.ViewOptional<const SpriteLayer>();

			for (EntityViewIterator entityIterator = view.begin(); entityIterator != view.end(); ++entityIterator)
			{
				TransformComponent& transform = transforms[*entityIterator];
				SpriteComponent& sprite = sprites[*entityIterator];

				auto entity = view.GetEntity(entityIterator.GetEntityIndex());
				if (!entity)
					continue;

				const SpriteLayer& layer = layers.GetOrDefault(*entityIterator, defaultSpriteLayer);

				m_SortedEntities.push_back({ entity.value(), layer.Layer });
			}
		}

		std::sort(m_SortedEntities.begin(), m_SortedEntities.end(), [](const std::pair<Entity, int32_t>& a, const std::pair<Entity, int32_t>& b) -> bool
		{
			return a.second < b.second;
		});

		const World& world = World::GetCurrent();
		for (const auto& [entity, layer] : m_SortedEntities)
		{
			const TransformComponent& transform = world.GetEntityComponent<TransformComponent>(entity);
			const SpriteComponent& sprite = world.GetEntityComponent<SpriteComponent>(entity);

			Renderer2D::DrawQuad(transform.GetTransformationMatrix(), sprite.Color,
				sprite.Texture == NULL_ASSET_HANDLE
				? nullptr
				: AssetManager::GetAsset<Texture>(sprite.Texture),
				sprite.TextureTiling, entity.GetIndex(),
				sprite.Flags);
		}
	}
}