#include "FlareScriptingCore/Flare.h"

#include "Flare/Core/Log.h"

#include <stdint.h>
#include <iostream>

#include <glm/glm.hpp>

namespace Sandbox
{
	struct Transform
	{
		FLARE_COMPONENT(Transform);

		glm::vec3 Position;
		glm::vec3 Rotation;
		glm::vec3 Scale;
	};
	FLARE_COMPONENT_ALIAS_IMPL(Transform, "struct Flare::TransformComponent");

	struct HealthComponent
	{
		FLARE_COMPONENT(HealthComponent);

		glm::vec2 Vector2;
		glm::vec3 Vector3;

		static void ConfigureSerialization(Flare::TypeSerializationSettings& settings)
		{
			FLARE_SERIALIZE_FIELD(settings, HealthComponent, Vector2);
			FLARE_SERIALIZE_FIELD(settings, HealthComponent, Vector3);
		}
	};
	FLARE_COMPONENT_IMPL(HealthComponent, HealthComponent::ConfigureSerialization);

	struct TestSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(TestSystem);

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
			config.Query.Add<Transform>();
			config.Query.Add<HealthComponent>();
		}

		virtual void Execute(Flare::EntityView& chunk) override
		{
			Flare::ComponentView<Transform> transforms = chunk.View<Transform>();

			for (Flare::EntityElement entity : chunk)
			{
				Transform& transform = transforms[entity];
				transform.Position += glm::vec3(0.1f, 0.0f, 0.0f) * Flare::Time::GetDeltaTime();
			}
		}
	};
	FLARE_SYSTEM_IMPL(TestSystem);
}