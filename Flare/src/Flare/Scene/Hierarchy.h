#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore/Serialization/SerializationStream.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/ComponentInitializer.h"
#include "FlareECS/System/SystemInitializer.h"

namespace Flare
{
	struct FLARE_API Children
	{
		FLARE_COMPONENT;

		std::vector<Entity> ChildrenEntities;
	};

	template<>
	struct TypeSerializer<Children>
	{
		static void OnSerialize(Children& children, SerializationStream& stream)
		{
			stream.Serialize("ChildrenEntities", SerializationValue(children.ChildrenEntities));
		}
	};



	struct FLARE_API Parent
	{
		FLARE_COMPONENT;

		Parent()
			: ParentEntity(Entity()) {}
		Parent(Entity parent)
			: ParentEntity(parent) {}

		Entity ParentEntity;
	};

	template<>
	struct TypeSerializer<Parent>
	{
		static void OnSerialize(Parent& parent, SerializationStream& stream)
		{
			stream.Serialize("ParentEntity", SerializationValue(parent.ParentEntity));
		}
	};

	//
	// HierarchyHelper
	//

	class FLARE_API HierarchyHelper
	{
	public:
		static void SetParent(World& world, Entity child, Entity parent);
		static void AddParent(World& world, Entity child, Entity parent);

		static void DeleteEntityHierarchy(World& world, Entity root);

		// `ignoredComponents` specifies which components should be initialized using a default constructor instead of copying them
		static Entity DuplicateEntityHierarchy(World& world, Entity root, const std::unordered_set<ComponentId>* ignoredComponents = nullptr);
	private:
		static Entity DuplicateEntity(World& world, Entity entity, const std::unordered_set<ComponentId>* ignoredComponents);
		static void RemoveFromParent(World& world, Entity child, Entity parent);
	};

	//
	// HierarchyProcessor
	//

	class HierarchyProcessor : public System
	{
	public:
		FLARE_SYSTEM;

		void OnConfig(World& world, SystemConfig& config) override;
		void OnUpdate(World& world, SystemExecutionContext& context) override;
	private:
		Query m_DeletedEntitiesWithParent;
		Query m_DeletedEntitiesWithChildren;
	};

	//
	// TransformPropagationSystem
	//

	struct TransformComponent;
	class FLARE_API TransformPropagationSystem : public System
	{
	public:
		FLARE_SYSTEM;

		void OnConfig(World& world, SystemConfig& config) override;
		void OnUpdate(World& world, SystemExecutionContext& context) override;

		static void PropagateTransformToChildren(World& world, Entity entity);
	private:
		static void PropagateTransformRecursively(World& world, const Children& children, const TransformComponent& parentTransform);
	private:
		Query m_Query;
	};
}
