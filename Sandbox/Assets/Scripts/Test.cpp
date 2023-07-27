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
}