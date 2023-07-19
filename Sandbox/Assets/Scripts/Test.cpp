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
	};
	FLARE_COMPONENT_IMPL(HealthComponent);

	struct TestSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(TestSystem);

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
		}

		virtual void Execute(Flare::EntityView& chunk) override
		{
			Flare::ComponentView<Transform> transforms = chunk.View<Transform>();

			for (Flare::EntityElement entity : chunk)
			{
				Transform& transform = transforms[entity];
				FLARE_INFO("Position: {0}, {1}, {2}", transform.Position.x, transform.Position.y, transform.Position.z);
			}
		}
	};
	FLARE_SYSTEM_IMPL(TestSystem);
}