#include "Hierarchy.h"

#include "FlareCore/Profiler/Profiler.h"

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
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("Late Update");
		FLARE_CORE_ASSERT(groupId.has_value());
		config.Group = *groupId;

		m_Query = world.NewQuery()
			.All()
			.With<TransformComponent, Children>()
			.Without<Parent>()
			.Build();
	}

	void TransformPropagationSystem::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		m_Query.ForEachChunk([&world](QueryChunk chunk,
			ComponentView<const TransformComponent> transforms,
			ComponentView<const Children> childrenComponents)
			{
				for (auto entity : chunk)
				{
					const TransformComponent& parentTransform = transforms[entity];
					const Children& children = childrenComponents[entity];

					glm::quat parentRotation = glm::quat(glm::radians(parentTransform.Rotation));

					for (Entity child : children.ChildrenEntities)
					{
						TransformComponent* globalTransform = world.TryGetEntityComponent<TransformComponent>(child);
						const LocalTransform* localTransform = world.TryGetEntityComponent<const LocalTransform>(child);

						if (!globalTransform || !localTransform)
							continue;

						glm::vec3 rotatedPosition = parentRotation * (localTransform->Position * parentTransform.Scale);
						globalTransform->Position = rotatedPosition + parentTransform.Position;
						globalTransform->Rotation = parentTransform.Rotation + localTransform->Rotation;
						globalTransform->Scale = parentTransform.Scale * localTransform->Scale;

						// TODO: Visit entities deeper in the hierarchy
					}
				}
			});
	}
}