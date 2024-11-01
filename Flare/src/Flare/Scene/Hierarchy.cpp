#include "PCH.h"

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

	void HierarchyHelper::DeleteEntityHierarchy(World& world, Entity root)
	{
		FLARE_PROFILE_FUNCTION();

		const Children* children = world.TryGetEntityComponent<const Children>(root);
		if (children == nullptr)
			return;

		for (Entity child : children->ChildrenEntities)
		{
			if (world.IsEntityAlive(child))
			{
				DeleteEntityHierarchy(world, child);
				world.DeleteEntity(child);
			}
		}

		if (world.IsEntityAlive(root))
		{
			world.DeleteEntity(root);
		}
	}

	Entity HierarchyHelper::DuplicateEntityHierarchy(World& world, Entity root, const std::unordered_set<ComponentId>* ignoredComponents)
	{
		FLARE_PROFILE_FUNCTION();

		Entity rootCopy = DuplicateEntityHierarchyRecursively(world, root, ignoredComponents);
		ReattachCopyToOriginalParent(world, root, rootCopy);

		return rootCopy;
	}

	Entity HierarchyHelper::DuplicateEntityHierarchyRecursively(World& world, Entity root, const std::unordered_set<ComponentId>* ignoredComponents)
	{
		FLARE_PROFILE_FUNCTION();
		FLARE_CORE_ASSERT(world.IsEntityAlive(root));

		Entity rootCopy = DuplicateEntity(world, root, ignoredComponents);

		const Children* children = world.TryGetEntityComponent<const Children>(root);

		if (children)
		{
			Children* copyChildren = world.TryGetEntityComponent<Children>(rootCopy);
			FLARE_CORE_ASSERT(copyChildren);

			for (size_t i = 0; i < children->ChildrenEntities.size(); i++)
			{
				Entity childCopy = DuplicateEntityHierarchyRecursively(world, children->ChildrenEntities[i], ignoredComponents);
				copyChildren->ChildrenEntities[i] = childCopy;

				Parent* parent = world.TryGetEntityComponent<Parent>(childCopy);
				if (parent)
				{
					parent->ParentEntity = rootCopy;
				}
			}
		}

		return rootCopy;
	}

	void HierarchyHelper::ReattachCopyToOriginalParent(World& world, Entity originalEntity, Entity copy)
	{
		FLARE_PROFILE_FUNCTION();
		
		const Parent* rootParent = world.TryGetEntityComponent<const Parent>(originalEntity);
		if (rootParent)
		{
			Children* parentChildren = world.TryGetEntityComponent<Children>(rootParent->ParentEntity);
			FLARE_CORE_ASSERT(parentChildren);

			auto originalRootIndex = std::find(parentChildren->ChildrenEntities.begin(), parentChildren->ChildrenEntities.end(), originalEntity);
			FLARE_CORE_ASSERT(originalRootIndex != parentChildren->ChildrenEntities.end());

			parentChildren->ChildrenEntities.insert(originalRootIndex + 1, copy);
		}
	}

	Entity HierarchyHelper::DuplicateEntity(World& world, Entity entity, const std::unordered_set<ComponentId>* ignoredComponents)
	{
		FLARE_PROFILE_FUNCTION();
		Entities& entities = world.Entities;
		ArchetypeId archetypeId = entities.GetEntityArchetype(entity);

		// Create an entity of the same archetype, but don't initialize any components.
		// They are manually initialized using a copy or a default constructor
		Entity duplicated = entities.CreateEntityFromArchetype(archetypeId, ComponentInitializationStrategy::NoInitialization);

		const ArchetypeRecord& archetype = world.GetArchetypes()[archetypeId];
		const ArchetypeComponents& archetypeComponents = world.GetArchetypes().GetArchetypeComponents(archetypeId);
		for (size_t i = 0; i < archetypeComponents.ComponentCount; i++)
		{
			const ComponentInfo& component = world.Components.GetComponentInfo(archetypeComponents.ComponentIds[i]);
			void* componentDestination = entities.GetEntityComponent(duplicated, archetypeComponents.ComponentIds[i]);

			FLARE_CORE_ASSERT(componentDestination);

			if (ignoredComponents && ignoredComponents->contains(archetypeComponents.ComponentIds[i]))
			{
				// Default initialize the ignored component
				component.Initializer->Type.Functions.DefaultConstructor(componentDestination);
			}
			else
			{
				const void* componentSource = entities.GetEntityComponent(entity, archetypeComponents.ComponentIds[i]);

				FLARE_CORE_ASSERT(componentSource);

				component.Initializer->Type.Functions.CopyConstructor(componentDestination, componentSource);
			}
		}

		return duplicated;
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

		m_DeletedEntitiesWithParent = world.NewQuery().Deleted().With<Parent>().Build();
		m_DeletedEntitiesWithChildren = world.NewQuery().Deleted().With<Children>().Build();
	}

	void HierarchyProcessor::OnUpdate(World& world, SystemExecutionContext& context)
	{
		FLARE_PROFILE_FUNCTION();

		m_DeletedEntitiesWithParent.ForEachChunk([&world](QueryChunk chunk, ComponentView<const Parent> parents)
			{
				uint32_t entityIndex = 0;
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					Entity parentEntity = parents[entityIndex].ParentEntity;
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

		m_DeletedEntitiesWithChildren.ForEachChunk([this, &world](QueryChunk chunk, ComponentView<const Children> childrenComponents)
			{
				uint32_t entityIndex = 0;
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					const Children& children = childrenComponents[entityIndex];

					for (Entity child : children.ChildrenEntities)
					{
						if (!world.IsEntityAlive(child))
							return;

						HierarchyHelper::DeleteEntityHierarchy(world, child);

						world.Entities.DeleteEntity(child, true);
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

		m_Query.ForEachChunk([this, &world](QueryChunk chunk,
			ComponentView<const TransformComponent> transforms,
			ComponentView<const Children> childrenComponents)
			{
				for (size_t entityIndex = 0; entityIndex < chunk.GetEntityCount(); entityIndex++)
				{
					const TransformComponent& parentTransform = transforms[entityIndex];
					const Children& children = childrenComponents[entityIndex];

					glm::quat parentRotation = glm::quat(glm::radians(parentTransform.Rotation));

					for (Entity child : children.ChildrenEntities)
					{
						TransformComponent* globalTransform = world.TryGetEntityComponent<TransformComponent>(child);
						const LocalTransform* localTransform = world.TryGetEntityComponent<const LocalTransform>(child);

						if (globalTransform && localTransform)
						{
							glm::vec3 rotatedPosition = parentRotation * (localTransform->Position * parentTransform.Scale);
							globalTransform->Position = rotatedPosition + parentTransform.Position;
							globalTransform->Rotation = parentTransform.Rotation + localTransform->Rotation;
							globalTransform->Scale = parentTransform.Scale * localTransform->Scale;
						}

						const Children* childrenEntities = world.TryGetEntityComponent<const Children>(child);
						if (childrenEntities && globalTransform)
						{
							PropagateTransformRecursively(world, *childrenEntities, *globalTransform);
						}
					}
				}
			});
	}

	void TransformPropagationSystem::PropagateTransformToChildren(World& world, Entity entity)
	{
		FLARE_PROFILE_FUNCTION();

		const Children* children = world.TryGetEntityComponent<const Children>(entity);
		const TransformComponent* globalTransform = world.TryGetEntityComponent<const TransformComponent>(entity);

		if (children && globalTransform)
		{
			PropagateTransformRecursively(world, *children, *globalTransform);
		}
	}

	void TransformPropagationSystem::PropagateTransformRecursively(World& world, const Children& children, const TransformComponent& parentTransform)
	{
		FLARE_PROFILE_FUNCTION();
		
		glm::quat parentRotation = glm::quat(glm::radians(parentTransform.Rotation));

		for (Entity child : children.ChildrenEntities)
		{
			const LocalTransform* localTransform = world.TryGetEntityComponent<const LocalTransform>(child);
			TransformComponent* globalTransform = world.TryGetEntityComponent<TransformComponent>(child);

			if (!localTransform || !globalTransform)
				continue;

			glm::vec3 rotatedPosition = parentRotation * (localTransform->Position * parentTransform.Scale);
			globalTransform->Position = rotatedPosition + parentTransform.Position;
			globalTransform->Rotation = parentTransform.Rotation + localTransform->Rotation;
			globalTransform->Scale = parentTransform.Scale * localTransform->Scale;

			const Children* childrenEntities = world.TryGetEntityComponent<const Children>(child);
			if (childrenEntities)
			{
				PropagateTransformRecursively(world, *childrenEntities, *globalTransform);
			}
		}
	}
}