#include <FlareECS/World.h>
#include <FlareECS/System/System.h>

#include <FlareECS/System/SystemInitializer.h>
#include <FlareECS/Entity/ComponentInitializer.h>

#include <FlareECS/Commands/CommandBuffer.h>

#include <Flare/Core/Time.h>
#include <FlareCore/Serialization/TypeInitializer.h>
#include <Flare/Scene/Components.h>

#include <Flare/Input/InputManager.h>

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

	struct SomeComponent
	{
		FLARE_COMPONENT;

		int a, b;

		SomeComponent()
			: a(100), b(-234) {}
	};
	FLARE_IMPL_COMPONENT(SomeComponent,
		FLARE_FIELD(SomeComponent, a),
		FLARE_FIELD(SomeComponent, b),
	);

	class RotatingQuadSystem : public Flare::System
	{
	public:
		FLARE_SYSTEM;

		RotatingQuadSystem() {}

		virtual void OnConfig(SystemConfig& config) override
		{
			m_Query = World::GetCurrent().CreateQuery<With<TransformComponent>, With<SpriteComponent>>();
			m_SingletonQuery = World::GetCurrent().CreateQuery<With<RotatingQuadData>>();
		}

		virtual void OnUpdate(SystemExecutionContext& context) override
		{
			const RotatingQuadData& data = World::GetCurrent().GetSingletonComponent<RotatingQuadData>();

			uint32_t op = 0;
			if (InputManager::IsKeyPressed(KeyCode::D1))
				op = 1;
			if (InputManager::IsKeyPressed(KeyCode::D2))
				op = 2;
			if (InputManager::IsKeyPressed(KeyCode::D3))
				op = 3;
			if (InputManager::IsKeyPressed(KeyCode::D4))
				op = 4;
			if (InputManager::IsKeyPressed(KeyCode::D5))
				op = 5;
			if (InputManager::IsKeyPressed(KeyCode::D6))
				op = 6;
			if (InputManager::IsKeyPressed(KeyCode::D7))
				op = 7;

			for (EntityView chunk : m_Query)
			{
				auto transforms = chunk.View<TransformComponent>();
				uint32_t entityIndex = 0;
				for (EntityViewElement entity : chunk)
				{
					Entity e = chunk.GetEntity(entityIndex).value_or(Entity());
					switch (op)
					{
					case 1:
						context.Commands->AddComponent<SomeComponent>(e);
						break;
					case 2:
						context.Commands->RemoveComponent<SomeComponent>(e);
						break;
					case 3:
						context.Commands->CreateEntity<TransformComponent, SpriteComponent, SomeComponent>();
						break;
					case 4:
						context.Commands->DeleteEntity(e);
						break;
					}

					transforms[entity].Rotation.z += data.RotationSpeed * Time::GetDeltaTime();

					entityIndex++;
				}
			}
		}
	private:
		Query m_Query;
		Query m_SingletonQuery;
	};
	FLARE_IMPL_SYSTEM(RotatingQuadSystem);
}
