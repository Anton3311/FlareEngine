#include <FlareECS/World.h>
#include <FlareECS/System/System.h>

#include <FlareECS/System/SystemInitializer.h>
#include <FlareECS/Entity/ComponentInitializer.h>

#include <Flare/Serialization/TypeInitializer.h>
#include <Flare/Scene/Components.h>

#include <iostream>


namespace Sandbox
{
	using namespace Flare;
	class SandboxTestSystem : public Flare::System
	{
	public:
		FLARE_SYSTEM;
		FLARE_TYPE;

		virtual void OnConfig(SystemConfig& config) override
		{
			m_TestQuery = World::GetCurrent().CreateQuery<With<TransformComponent>, Without<CameraComponent>>();
		}

		virtual void OnUpdate(SystemExecutionContext& context) override
		{
			for (EntityView chunk : m_TestQuery)
			{
				auto transforms = chunk.View<TransformComponent>();
				for (EntityViewElement entity : chunk)
				{
					transforms[entity].Rotation.z += 2.0f;
				}
			}
		}

		int a;
		float b;
	private:
		Query m_TestQuery;
	};
	FLARE_IMPL_SYSTEM(SandboxTestSystem);
	FLARE_IMPL_TYPE(SandboxTestSystem,
		FLARE_FIELD(SandboxTestSystem, a),
		FLARE_FIELD(SandboxTestSystem, b)
	);

	struct SomeComponent
	{
		FLARE_COMPONENT2;

		int a, b;
	};
	FLARE_IMPL_COMPONENT2(SomeComponent,
		FLARE_FIELD(SomeComponent, a),
		FLARE_FIELD(SomeComponent, b),
	);
}
