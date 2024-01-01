#include "Sandbox.h"

#include <FlareECS/World.h>
#include <FlareECS/System/System.h>

#include <FlareECS/Commands/CommandBuffer.h>

#include <Flare/Core/Time.h>

#include <Flare/Scene/Components.h>
#include <Flare/Scene/Prefab.h>

#include <Flare/Input/InputManager.h>

#include <iostream>
#include <random>

namespace Sandbox
{
	FLARE_IMPL_COMPONENT(RotatingQuadData);
	FLARE_IMPL_COMPONENT(SomeComponent);
	FLARE_IMPL_COMPONENT(TestComponent);

	class RotatingQuadSystem : public Flare::System
	{
	public:
		FLARE_SYSTEM;

		RotatingQuadSystem() {}

		virtual void OnConfig(World& world, SystemConfig& config) override
		{
			m_Query = world.NewQuery().With<TransformComponent, SpriteComponent>().Create();
			m_SingletonQuery = world.NewQuery().With<RotatingQuadData>().Create();

			m_DeletionQuery = world.NewQuery()
				.Deleted()
				.With<SomeComponent>()
				.Create();
		}

		virtual void OnUpdate(World& world, SystemExecutionContext& context) override
		{
			std::optional<const RotatingQuadData*> data = World::GetCurrent().TryGetSingletonComponent<const RotatingQuadData>();
			if (data.has_value())
			{
				Ref<Prefab> prefab = AssetManager::GetAsset<Prefab>(data.value()->PrefabHandle);

				static std::random_device s_Device;
				static std::mt19937_64 s_Engine(s_Device());
				static std::uniform_real_distribution<float> s_UniformDistricution(-50.0f, 50.0f);

				uint32_t op = 0;
				if (InputManager::IsKeyPressed(KeyCode::D1))
					op = 1;
				if (InputManager::IsKeyPressed(KeyCode::D2))
					op = 2;
				if (InputManager::IsKeyPressed(KeyCode::D3))
					op = 3;
				if (InputManager::IsKeyPressed(KeyCode::D4))
					op = 4;
				if (InputManager::IsKeyPressed(KeyCode::D5))
					op = 5;
				if (InputManager::IsKeyPressed(KeyCode::D6))
					op = 6;
				if (InputManager::IsKeyPressed(KeyCode::D7))
					op = 7;

				for (EntityView chunk : m_Query)
				{
					auto transforms = chunk.View<TransformComponent>();
					uint32_t entityIndex = 0;
					for (EntityViewElement entity : chunk)
					{
						Entity e = chunk.GetEntity(entityIndex).value_or(Entity());
						switch (op)
						{
						case 1:
						{
							glm::vec3 position = glm::vec3(s_UniformDistricution(s_Engine), s_UniformDistricution(s_Engine), 0.0f);
							context.Commands->AddEntityCommand(InstantiatePrefab(prefab))
								.AddComponent<SomeComponent>()
								.SetComponent<TransformComponent>(TransformComponent{ position });
							break;
						}
						case 2:
							context.Commands->GetEntity(e)
								.AddComponent<SomeComponent>();
							break;
						case 3:
							context.Commands->CreateEntity<TransformComponent, SpriteComponent, SomeComponent>();
							break;
						case 4:
							context.Commands->DeleteEntity(e);
							break;
						case 5:
							context.Commands->CreateEntity(TransformComponent{ glm::vec3(1.0f, 0.0f, 0.0f) }, SomeComponent{});
							break;
						case 6:
							context.Commands->GetEntity(e)
								.RemoveComponent<SomeComponent>();
							break;
						}

						transforms[entity].Rotation.z += data.value()->RotationSpeed * Time::GetDeltaTime();

						entityIndex++;
					}
				}
			}

			for (EntityView view : m_DeletionQuery)
			{
				for (EntityViewIterator entity = view.begin(); entity != view.end(); ++entity)
				{
					auto id = view.GetEntity(entity.GetEntityIndex());
					if (id.has_value())
						FLARE_INFO("{}", id->GetIndex());
				}
			}
		}
	private:
		Query m_Query;
		Query m_SingletonQuery;
		Query m_DeletionQuery;
	};
	//FLARE_IMPL_SYSTEM(RotatingQuadSystem);
}
