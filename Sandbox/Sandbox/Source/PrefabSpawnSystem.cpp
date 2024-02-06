#include "PrefabSpawnSystem.h"

#include <FlareECS/World.h>

#include <Flare/Core/Time.h>
#include <Flare/Scene/Prefab.h>
#include <Flare/Scene/Components.h>
#include <Flare/Scene/Transform.h>

#include <random>

using namespace Flare;

FLARE_IMPL_COMPONENT(PrefabSpawner);

void PrefabSpawnSystem::OnConfig(Flare::World& world, SystemConfig& config)
{
	m_Query = world.NewQuery()
		.With<PrefabSpawner>()
		.Create();
}

void PrefabSpawnSystem::OnUpdate(Flare::World& world, SystemExecutionContext& context)
{
	static std::random_device s_Device;
	static std::mt19937_64 s_Engine(s_Device());
	static std::uniform_real_distribution<float> s_UniformDistricution(-50.0f, 50.0f);

	for (EntityView view : m_Query)
	{
		auto spawners = view.View<PrefabSpawner>();

		for (EntityViewElement entity : view)
		{
			PrefabSpawner& spawner = spawners[entity];
			
			if (!spawner.Enabled || spawner.PrefabHandle == NULL_ASSET_HANDLE)
			{
				continue;
			}

			spawner.TimeLeft -= Time::GetDeltaTime();
				
			if (spawner.TimeLeft <= 0.0f)
			{
				if (!AssetManager::IsAssetHandleValid(spawner.PrefabHandle))
				{
					FLARE_WARN("Invalid prefab handle {}", (uint64_t)spawner.PrefabHandle);
					continue;
				}

				glm::vec3 position = glm::vec3(
					s_UniformDistricution(s_Engine),
					s_UniformDistricution(s_Engine),
					s_UniformDistricution(s_Engine)
				);

				Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(spawner.PrefabHandle);
				context.Commands->AddEntityCommand(InstantiatePrefab(prefab))
					.SetComponent(TransformComponent(position));

				spawner.TimeLeft = spawner.Period;
			}
		}
	}
}

FLARE_IMPL_SYSTEM(PrefabSpawnSystem);