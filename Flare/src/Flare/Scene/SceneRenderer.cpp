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
		const MaterialComponent defaultMaterial = NULL_ASSET_HANDLE;
		
		for (EntityView view : m_SpritesQuery)
		{
			ComponentView<TransformComponent> transforms = view.View<TransformComponent>();
			ComponentView<SpriteComponent> sprites = view.View<SpriteComponent>();
			auto layers = view.ViewOptional<const SpriteLayer>();
			auto materials = view.ViewOptional<const MaterialComponent>();

			for (EntityViewIterator entityIterator = view.begin(); entityIterator != view.end(); ++entityIterator)
			{
				TransformComponent& transform = transforms[*entityIterator];
				SpriteComponent& sprite = sprites[*entityIterator];

				auto entity = view.GetEntity(entityIterator.GetEntityIndex());
				if (!entity)
					continue;

				const SpriteLayer& layer = layers.GetOrDefault(*entityIterator, defaultSpriteLayer);
				const MaterialComponent& material = materials.GetOrDefault(*entityIterator, defaultMaterial);

				m_SortedEntities.push_back({ entity.value(), layer.Layer, material.Material });
			}
		}

		std::sort(m_SortedEntities.begin(), m_SortedEntities.end(), [](const EntityQueueElement& a, const EntityQueueElement& b) -> bool
		{
			if (a.SortingLayer == b.SortingLayer)
				return (size_t)a.Material < (size_t)b.Material;
			return a.SortingLayer < b.SortingLayer;
		});

		const World& world = World::GetCurrent();
		AssetHandle currentMaterial = NULL_ASSET_HANDLE;

		for (const auto& [entity, layer, material] : m_SortedEntities)
		{
			const TransformComponent& transform = world.GetEntityComponent<TransformComponent>(entity);
			const SpriteComponent& sprite = world.GetEntityComponent<SpriteComponent>(entity);

			if (material != currentMaterial)
			{
				Ref<Material> materialInstance = AssetManager::GetAsset<Material>(material);
				currentMaterial = material;

				if (materialInstance)
				{
					Renderer2D::SetMaterial(materialInstance);
				}
				else
					Renderer2D::SetMaterial(nullptr);
			}

			Renderer2D::DrawQuad(transform.GetTransformationMatrix(), sprite.Color,
				sprite.Texture == NULL_ASSET_HANDLE
				? nullptr
				: AssetManager::GetAsset<Texture>(sprite.Texture),
				sprite.TextureTiling, entity.GetIndex(),
				sprite.Flags);
		}
	}
}