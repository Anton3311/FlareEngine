#include "FlareScriptingCore/Flare.h"

#include "Flare/Core/Log.h"

#include <stdint.h>
#include <iostream>

#include <glm/glm.hpp>

namespace Sandbox
{
	struct MovingQuadComponet
	{
		FLARE_COMPONENT(MovingQuadComponet);

		float Speed;
		
		static void ConfigureSerialization(Flare::TypeSerializationSettings& settings)
		{
			FLARE_SERIALIZE_FIELD(settings, MovingQuadComponet, Speed);
		}
	};
	FLARE_COMPONENT_IMPL(MovingQuadComponet, MovingQuadComponet::ConfigureSerialization);

	struct TestComponent
	{
		FLARE_COMPONENT(TestComponent);

		static void ConfigureSerialization(Flare::TypeSerializationSettings& settings)
		{
		}
	};
	FLARE_COMPONENT_IMPL(TestComponent, TestComponent::ConfigureSerialization);

	struct TestSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(TestSystem);

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
			config.Query.With<Flare::Transform>();
			config.Query.With<MovingQuadComponet>();
			config.Query.Without<TestComponent>();
		}

		static void ConfigureSerialization(Flare::TypeSerializationSettings& settings)
		{
		}

		virtual void Execute(Flare::EntityView& chunk) override
		{
			glm::vec3 direction = glm::vec3(0.0f);

			if (Flare::Input::IsKeyPressed(Flare::KeyCode::W))
				direction.y += 1;
			if (Flare::Input::IsKeyPressed(Flare::KeyCode::S))
				direction.y -= 1;
			if (Flare::Input::IsKeyPressed(Flare::KeyCode::A))
				direction.x -= 1;
			if (Flare::Input::IsKeyPressed(Flare::KeyCode::D))
				direction.x += 1;

			Flare::ComponentView<Flare::Transform> transforms = chunk.View<Flare::Transform>();
			Flare::ComponentView<Flare::Sprite> sprites = chunk.View<Flare::Sprite>();
			Flare::ComponentView<MovingQuadComponet> quadComponents = chunk.View<MovingQuadComponet>();

			for (Flare::EntityElement entity : chunk)
			{
				Flare::Transform& transform = transforms[entity];
				Flare::Sprite& sprite = sprites[entity];

				transform.Position += direction * quadComponents[entity].Speed * Flare::Time::GetDeltaTime();
			}
		}
	};
	FLARE_SYSTEM_IMPL(TestSystem);

	struct Spawner
	{
		FLARE_COMPONENT(Spawner);
		float Interval;
		float TimeLeft;

		static void ConfigureSerialization(Flare::TypeSerializationSettings& settings)
		{
			FLARE_SERIALIZE_FIELD(settings, Spawner, Interval);
			FLARE_SERIALIZE_FIELD(settings, Spawner, TimeLeft);
		}
	};
	FLARE_COMPONENT_IMPL(Spawner, Spawner::ConfigureSerialization);

	struct SpawnSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(SpawnSystem);

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
			config.Query.With<Spawner>();
		}

		static void ConfigureSerialization(Flare::TypeSerializationSettings& settings)
		{
		}

		virtual void Execute(Flare::EntityView& chunk) override
		{
			Flare::ComponentView<Spawner> spawners = chunk.View<Spawner>();

			bool spawn = false;
			for (Flare::EntityElement entity : chunk)
			{
				Spawner& spawner = spawners[entity];

				if (spawner.TimeLeft >= 0)
					spawner.TimeLeft -= Flare::Time::GetDeltaTime();
				else
				{
					spawn = true;
					spawner.TimeLeft += spawner.Interval;
				}
			}

			if (spawn)
			{
				Flare::Entity entity = Flare::World::CreateEntity<Flare::Transform, Flare::Sprite, MovingQuadComponet>();
				Flare::Transform& transform = Flare::World::GetEntityComponent<Flare::Transform>(entity);
				transform.Scale = glm::vec3(1.0f);

				Flare::Sprite& sprite = Flare::World::GetEntityComponent<Flare::Sprite>(entity);
				sprite.Color = glm::vec4(1.0f);

				MovingQuadComponet& movingQuad = Flare::World::GetEntityComponent<MovingQuadComponet>(entity);
				movingQuad.Speed = 10.0f;
			}
		}
	};
	FLARE_SYSTEM_IMPL(SpawnSystem);
}