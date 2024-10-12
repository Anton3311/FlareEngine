#include "PrefabSpawnSystem.h"

#include <FlareCore/Log.h>

#include <Flare/Core/Time.h>
#include <Flare/Scene/Prefab.h>
#include <Flare/Scene/Components.h>
#include <Flare/Scene/Transform.h>

#include <FlareECS/World.h>

#include <random>

using namespace Flare;

FLARE_IMPL_COMPONENT(PrefabSpawner);

void PrefabSpawnSystem::OnConfig(Flare::World& world, SystemConfig& config)
{
	m_Query = world.NewQuery()
		.All()
		.With<PrefabSpawner>()
		.Build();
}

void PrefabSpawnSystem::OnUpdate(Flare::World& world, SystemExecutionContext& context)
{
	static std::random_device s_Device;
	static std::mt19937_64 s_Engine(s_Device());
	static std::uniform_real_distribution<float> s_UniformDistribution(-50.0f, 50.0f);

	m_Query.ForEachChunk([&](QueryChunk chunk, ComponentView<PrefabSpawner> prefabSpawners)
		{
			for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
			{
				PrefabSpawner& spawner = prefabSpawners[entityIndex];

				if (!spawner.Enabled || spawner.PrefabHandle == NULL_ASSET_HANDLE)
					continue;

				spawner.TimeLeft -= Time::GetDeltaTime();

				if (spawner.TimeLeft <= 0.0f)
				{
					if (!AssetManager::IsAssetHandleValid(spawner.PrefabHandle))
					{
						FLARE_WARN("Invalid prefab handle {}", (uint64_t)spawner.PrefabHandle);
						continue;
					}

					glm::vec3 position = glm::vec3(
						s_UniformDistribution(s_Engine),
						s_UniformDistribution(s_Engine),
						s_UniformDistribution(s_Engine)
					);

					Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(spawner.PrefabHandle);
					context.Commands->AddEntityCommand(InstantiatePrefab(prefab))
						.SetComponent(TransformComponent(position));

					spawner.TimeLeft = spawner.Period;
				}
			}
		});
}

FLARE_IMPL_SYSTEM(PrefabSpawnSystem);