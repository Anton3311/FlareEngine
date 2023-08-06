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

	struct TestSingleton
	{
		FLARE_COMPONENT(TestSingleton);

		float Num;

		static void ConfigureSerialization(Flare::TypeSerializationSettings& settings)
		{
			FLARE_SERIALIZE_FIELD(settings, TestSingleton, Num);
		}
	};
	FLARE_COMPONENT_IMPL(TestSingleton, TestSingleton::ConfigureSerialization);

	struct TestSystem : public Flare::SystemBase
	{
		FLARE_SYSTEM(TestSystem);

		virtual void Configure(Flare::SystemConfiguration& config) override
		{
			m_TestQuery = Flare::Query::New<Flare::With<Flare::Transform>, Flare::With<MovingQuadComponet>>();
			m_SingletonQuery = Flare::Query::New<Flare::With<TestSingleton>>();
		}

		static void ConfigureSerialization(Flare::TypeSerializationSettings& settings)
		{
		}

		virtual void OnUpdate() override
		{
			TestSingleton* singleton = Flare::World::GetSingletonComponent<TestSingleton>();
			if (singleton != nullptr)
				FLARE_INFO(singleton->Num);

			Flare::World::ForEachChunk(m_TestQuery, [](Flare::EntityView& view)
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

				Flare::ComponentView<Flare::Transform> transforms = view.View<Flare::Transform>();
				Flare::ComponentView<Flare::Sprite> sprites = view.View<Flare::Sprite>();
				Flare::ComponentView<MovingQuadComponet> quadComponents = view.View<MovingQuadComponet>();
					
				for (Flare::EntityElement entity : view)
				{
					Flare::Transform& transform = transforms[entity];
					Flare::Sprite& sprite = sprites[entity];

					transform.Position += direction * quadComponents[entity].Speed * Flare::Time::GetDeltaTime();
				}
			});
		}
	private:
		Flare::Query m_TestQuery;
		Flare::Query m_SingletonQuery;
	};
	FLARE_SYSTEM_IMPL(TestSystem);
}