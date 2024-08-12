#include "Hierarchy.h"

#include "FlareCore/Log.h"
#include "FlareCore/Profiler/Profiler.h"

#include "Flare/Scene/Transform.h"

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Flare
{
	FLARE_IMPL_COMPONENT(Children);
	FLARE_IMPL_COMPONENT(Parent);

	//
	// HierarchyHelper
	//

	void HierarchyHelper::SetParent(World& world, Entity child, Entity parent)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(world.IsEntityAlive(child) && world.IsEntityAlive(parent));

		Parent* childParent = world.TryGetEntityComponent<Parent>(child);
		if (childParent && childParent->ParentEntity == parent)
			return;
		
		if (!childParent)
		{
			AddParent(world, child, parent);
			return;
		}

		RemoveFromParent(world, child, childParent->ParentEntity);

		childParent->ParentEntity = parent;

		Children* children = world.TryGetEntityComponent<Children>(parent);
		if (!children)
		{
			world.AddEntityComponent<Children>(parent, Children());
			children = &world.GetEntityComponent<Children>(parent);
		}

		children->ChildrenEntities.push_back(child);
	}

	void HierarchyHelper::AddParent(World& world, Entity child, Entity parent)
	{
		FLARE_PROFILE_FUNCTION();

		world.AddEntityComponent(child, Parent(parent));

		Children* children = world.TryGetEntityComponent<Children>(parent);
		if (!children)
		{
			world.AddEntityComponent(parent, Children());
			children = &world.GetEntityComponent<Children>(parent);
		}

		children->ChildrenEntities.push_back(child);
	}

	void HierarchyHelper::RemoveFromParent(World& world, Entity child, Entity parent)
	{
		FLARE_PROFILE_FUNCTION();

		Children* children = world.TryGetEntityComponent<Children>(parent);
		if (!children)
			return;

		auto it = std::find(
			children->ChildrenEntities.begin(),
			children->ChildrenEntities.end(),
			child);

		if (it == children->ChildrenEntities.end())
			return;

		children->ChildrenEntities.erase(it);
	}

	//
	// HierarchyProcessor
	//

	FLARE_IMPL_SYSTEM(HierarchyProcessor);
	void HierarchyProcessor::OnConfig(World& world, SystemConfig& config)
	{
		FLARE_PROFILE_FUNCTION();

		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("SceneHierarchyUpdate");
		FLARE_CORE_ASSERT(groupId.has_value());
		config.Group = *groupId;

		config.ExecuteBefore<TransformPropagationSystem>();

		m_DeletedEntities = world.NewQuery().Deleted().With<Parent>().Build();
	}

	void HierarchyProcessor::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		m_DeletedEntities.ForEachChunk([&world](QueryChunk chunk, ComponentView<const Parent> parents)
			{
				uint32_t entityIndex = 0;
				for (auto entity : chunk)
				{
					Entity parentEntity = parents[entity].ParentEntity;
					Entity thisEntity = chunk.GetEntityId(entityIndex);

					{
						Children* children = world.TryGetEntityComponent<Children>(parentEntity);
						if (children)
						{
							auto it = std::find(children->ChildrenEntities.begin(), children->ChildrenEntities.end(), thisEntity);
							if (it != children->ChildrenEntities.end())
							{
								children->ChildrenEntities.erase(it);
							}
						}
					}

					entityIndex++;
				}
			});
	}

	//
	// TransformPropagationSystem
	//

	FLARE_IMPL_SYSTEM(TransformPropagationSystem);
	void TransformPropagationSystem::OnConfig(World& world, SystemConfig& config)
	{
		std::optional<uint32_t> groupId = world.GetSystemsManager().FindGroup("SceneHierarchyUpdate");
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