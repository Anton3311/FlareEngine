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

		glm::vec3 Velocity;
		
		static void ConfigureSerialization(Flare::TypeSerializationSettings& settings)
		{
			FLARE_SERIALIZE_FIELD(settings, MovingQuadComponet, Velocity);
		}
	};
	FLARE_COMPONENT_IMPL(MovingQuadComponet, MovingQuadComponet::ConfigureSerialization);

	struct TestSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(TestSystem);

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
			config.Query.Add<Flare::Transform>();
			config.Query.Add<MovingQuadComponet>();
			config.Query.Add<Flare::Sprite>();
		}

		virtual void Execute(Flare::EntityView& chunk) override
		{
			Flare::ComponentView<Flare::Transform> transforms = chunk.View<Flare::Transform>();
			Flare::ComponentView<Flare::Sprite> sprites = chunk.View<Flare::Sprite>();
			Flare::ComponentView<MovingQuadComponet> quadComponents = chunk.View<MovingQuadComponet>();

			for (Flare::EntityElement entity : chunk)
			{
				Flare::Transform& transform = transforms[entity];
				Flare::Sprite& sprite = sprites[entity];

				transform.Position += quadComponents[entity].Velocity * Flare::Time::GetDeltaTime();
				sprite.Color.r = glm::sin(transform.Position.x * 10.0f);
				sprite.Color.g = glm::cos(transform.Position.y * 10.0f);
			}
		}
	};
	FLARE_SYSTEM_IMPL(TestSystem);
}