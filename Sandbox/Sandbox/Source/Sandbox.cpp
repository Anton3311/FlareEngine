#include <Flare/Scene/Components.h>

#include <FlareECS/World.h>
#include <FlareECS/System/System.h>
#include <FlareECS/System/SystemInitializer.h>

#include <iostream>

namespace Sandbox
{
	using namespace Flare;
	class TestSandboxSystem : public System
	{
		FLARE_SYSTEM;
		virtual void OnConfig(SystemConfig& config) override
		{
			World& world = World::GetCurrent();
			const std::vector<SystemInitializer*>& inits = SystemInitializer::GetInitializers();
			
			config.Group = World::GetCurrent().GetSystemsManager().FindGroup("Scripting Update");

			m_TestQuery = World::GetCurrent().CreateQuery<With<TransformComponent>>();
		}

		virtual void OnUpdate(Flare::SystemExecutionContext& context) override
	 	{
			std::cout << "Update\n";
			for (EntityView chunk : m_TestQuery)
			{
				auto transforms = chunk.View<TransformComponent>();
				for (EntityViewElement entity : chunk)
				{
					transforms[entity].Rotation.z += 10.0f;
				}
			}
		}

	private:
		Query m_TestQuery;
	};
	FLARE_IMPL_SYSTEM(TestSandboxSystem);
}
