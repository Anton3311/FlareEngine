#include <FlareECS/World.h>
#include <FlareECS/System/System.h>

#include <FlareECS/System/SystemInitializer.h>
#include <FlareECS/Entity/ComponentInitializer.h>

#include <Flare/Core/Time.h>
#include <Flare/Serialization/TypeInitializer.h>
#include <Flare/Scene/Components.h>

#include <iostream>

namespace Sandbox
{
	using namespace Flare;
	struct RotatingQuadData
	{
		FLARE_COMPONENT;
		float RotationSpeed;
	};
	FLARE_IMPL_COMPONENT(RotatingQuadData,
		FLARE_FIELD(RotatingQuadData, RotationSpeed)
	);

	class RotatingQuadSystem : public Flare::System
	{
	public:
		FLARE_SYSTEM;

		virtual void OnConfig(SystemConfig& config) override
		{
			m_Query = World::GetCurrent().CreateQuery<With<TransformComponent>, Without<CameraComponent>>();
			m_SingletonQuery = World::GetCurrent().CreateQuery<With<RotatingQuadData>>();
		}

		virtual void OnUpdate(SystemExecutionContext& context) override
		{
			const RotatingQuadData& data = World::GetCurrent().GetSingletonComponent<RotatingQuadData>();
			Entity e = World::GetCurrent().GetSingletonEntity(m_SingletonQuery);

			for (EntityView chunk : m_Query)
			{
				auto transforms = chunk.View<TransformComponent>();
				for (EntityViewElement entity : chunk)
				{
					transforms[entity].Rotation.z += data.RotationSpeed * Time::GetDeltaTime();
				}
			}
		}
	private:
		Query m_Query;
		Query m_SingletonQuery;
	};
	FLARE_IMPL_SYSTEM(RotatingQuadSystem);
		
	struct SomeComponent
	{
		FLARE_COMPONENT;

		int a, b;
	};
	FLARE_IMPL_COMPONENT(SomeComponent,
		FLARE_FIELD(SomeComponent, a),
		FLARE_FIELD(SomeComponent, b),
	);
}
