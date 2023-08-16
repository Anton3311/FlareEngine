#include <FlareECS/World.h>
#include <FlareECS/System/System.h>
#include <FlareECS/System/SystemInitializer.h>

#include <Flare/Scene/Components.h>

#include <iostream>

namespace Sandbox
{
	using namespace Flare;
	class SandboxTestSystem : public Flare::System
	{
	public:
		FLARE_SYSTEM;

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
	private:
		Query m_TestQuery;
	};
	FLARE_IMPL_SYSTEM(SandboxTestSystem);
}
