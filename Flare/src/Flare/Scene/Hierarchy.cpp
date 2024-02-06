#include "Hierarchy.h"

#include "Flare/Scene/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Flare
{
	FLARE_IMPL_COMPONENT(Children);
	FLARE_IMPL_COMPONENT(Parent);



	FLARE_IMPL_SYSTEM(TransformPropagationSystem);
	void TransformPropagationSystem::OnConfig(World& world, SystemConfig& config)
	{
		config.Group = world.GetSystemsManager().FindGroup("Late Update");
		m_Qeury = world.NewQuery()
			.With<TransformComponent, Children>()
			.Without<Parent>()
			.Create();
	}

	void TransformPropagationSystem::OnUpdate(World& world, SystemExecutionContext& context)
	{
		for (auto view : m_Qeury)
		{
			auto transforms = view.View<TransformComponent>();
			auto childrenComponents = view.View<Children>();

			for (auto entity : view)
			{
				const TransformComponent& parentTransform = transforms[entity];
				const Children& children = childrenComponents[entity];

				glm::quat parentRotation = glm::quat(glm::radians(parentTransform.Rotation));

				for (Entity child : children.ChildrenEntities)
				{
					GlobalTransform* globalTransform = world.TryGetEntityComponent<GlobalTransform>(child);
					const TransformComponent* transform = world.TryGetEntityComponent<const TransformComponent>(child);

					if (!globalTransform || !transform)
						continue;

					glm::vec3 rotatedPosition = parentRotation * (transform->Position * parentTransform.Scale);
					globalTransform->Position = rotatedPosition + parentTransform.Position;
					globalTransform->Rotation = parentTransform.Rotation + transform->Rotation;
					globalTransform->Scale = parentTransform.Scale * transform->Scale;
				}
			}
		}
	}
}