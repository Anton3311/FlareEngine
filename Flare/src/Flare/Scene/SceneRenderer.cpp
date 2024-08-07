#include "SceneRenderer.h"

#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Renderer/Renderer.h"
#include "Flare/Renderer/MaterialsTable.h"

#include "Flare/Scene/Transform.h"

#include <algorithm>

namespace Flare
{
	void SpriteRendererSystem::OnConfig(World& world, SystemConfig& config)
	{
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("Rendering");
		FLARE_CORE_ASSERT(groupId);
		config.Group = *groupId;

		m_SpritesQuery = world.NewQuery().All().With<TransformComponent, SpriteComponent>().Build();
		m_TextQuery = world.NewQuery().All().With<TransformComponent, TextComponent>().Build();
	}

	void SpriteRendererSystem::OnUpdate(World& world, SystemExecutionContext& context)
	{
		RenderQuads(world, context);
		RenderText(context);
	}

	void SpriteRendererSystem::RenderQuads(World& world, SystemExecutionContext& context)
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

			Renderer2D::DrawSprite(sprite.Sprite, transform.GetTransformationMatrix(), sprite.Color, sprite.Tilling, sprite.Flags, entity.GetIndex());
		}
	}

	void SpriteRendererSystem::RenderText(SystemExecutionContext& context)
	{
		for (EntityView view : m_TextQuery)
		{
			auto transforms = view.View<TransformComponent>();
			auto texts = view.View<TextComponent>();

			for (EntityViewIterator entity = view.begin(); entity != view.end(); ++entity)
			{
				glm::mat4 transform = transforms[*entity].GetTransformationMatrix();

				Entity entityId = view.GetEntity(entity.GetEntityIndex()).value_or(Entity());
				const TextComponent& text = texts[*entity];

				Renderer2D::DrawString(
					text.Text, transform,
					text.Font ? text.Font : Font::GetDefault(),
					text.Color, entityId.GetIndex());
			}
		}
	}

	// Mesh Renderer

	void MeshRendererSystem::OnConfig(World& world, SystemConfig& config)
	{
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("Rendering");
		FLARE_CORE_ASSERT(groupId);
		config.Group = *groupId;

		m_Query = world.NewQuery().All().With<TransformComponent, MeshComponent>().Build();
	}

	void MeshRendererSystem::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		AssetHandle currentMaterialHandle = NULL_ASSET_HANDLE;
		Ref<Material> currentMaterial = nullptr;
		Ref<MaterialsTable> currentMaterialsTable = nullptr;
		Ref<Material> errorMaterial = Renderer::GetErrorMaterial();

		RendererSubmitionQueue& submitionQueue = Renderer::GetOpaqueSubmitionQueue();

		bool isMaterialTable = false;

		for (EntityView view : m_Query)
		{
			auto transforms = view.View<TransformComponent>();
			auto meshes = view.View<MeshComponent>();

			for (EntityViewIterator entity = view.begin(); entity != view.end(); ++entity)
			{
				const Ref<Mesh>& mesh = meshes[*entity].Mesh;
				const TransformComponent& transform = transforms[*entity];
				std::optional<Entity> id = view.GetEntity(entity.GetEntityIndex());

				if (!mesh || !id)
					continue;

				if (meshes[*entity].Material != currentMaterialHandle)
				{
					const AssetMetadata* meta = AssetManager::GetAssetMetadata(meshes[*entity].Material);
					if (!meta)
						continue;

					if (meta->Type == AssetType::Material)
					{
						currentMaterial = AssetManager::GetAsset<Material>(meshes[*entity].Material);
						isMaterialTable = false;
					}
					else if (meta->Type == AssetType::MaterialsTable)
					{
						currentMaterialsTable = AssetManager::GetAsset<MaterialsTable>(meshes[*entity].Material);
						isMaterialTable = true;
					}

					currentMaterialHandle = meshes[*entity].Material;
				}

				if (!isMaterialTable)
				{
					submitionQueue.Submit(mesh,
						currentMaterial,
						Math::Compact3DTransform(transform.GetTransformationMatrix()),
						meshes[*entity].Flags);
				}
				else
				{
					submitionQueue.Submit(mesh,
						Span<AssetHandle>::FromVector(currentMaterialsTable->Materials),
						Math::Compact3DTransform(transform.GetTransformationMatrix()),
						meshes[*entity].Flags);
				}
			}
		}
	}



	void DecalRendererSystem::OnConfig(World& world, SystemConfig& config)
	{
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("Rendering");
		FLARE_CORE_ASSERT(groupId);
		config.Group = *groupId;

		m_DecalsQuery = world.NewQuery().All().With<TransformComponent, Decal>().Build();
	}

	void DecalRendererSystem::OnUpdate(World& world, SystemExecutionContext& context)
	{
		m_DecalsQuery.ForEachChunk([](QueryChunk chunk,
			ComponentView<const TransformComponent> transforms,
			ComponentView<const Decal> decals)
			{
				for (auto entity : chunk)
				{
					Renderer::SubmitDecal(decals[entity].Material, transforms[entity].GetTransformationMatrix());
				}
			});
	}
}