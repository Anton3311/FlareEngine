#pragma once

#include "FlareCore/Serialization/TypeSerializer.h"
#include "FlareCore/Serialization/SerializationStream.h"

#include "FlareECS/World.h"
#include "FlareECS/Entity/ComponentInitializer.h"
#include "FlareECS/System/SystemInitializer.h"

namespace Flare
{
	class HierarchyHelper;
	class HierarchyProcessor;



	struct FLARE_API Children
	{
	public:
		FLARE_COMPONENT;

		const std::vector<Entity>& GetChildren() const { return m_ChildrenEntities; }
	private:
		std::vector<Entity> m_ChildrenEntities;

		friend struct TypeSerializer<Children>;
		friend class HierarchyHelper;
		friend class HierarchyProcessor;
	};

	template<>
	struct TypeSerializer<Children>
	{
		static void OnSerialize(Children& children, SerializationStream& stream)
		{
			stream.Serialize("ChildrenEntities", SerializationValue(children.m_ChildrenEntities));
		}
	};



	struct FLARE_API Parent
	{
	public:
		FLARE_COMPONENT;

		Parent()
			: m_ParentEntity(Entity()) {}
		Parent(Entity parent)
			: m_ParentEntity(parent) {}

		inline Entity GetParentEntity() const { return m_ParentEntity; }
	private:
		Entity m_ParentEntity;

		friend struct TypeSerializer<Parent>;
		friend class HierarchyHelper;
		friend class HierarchyProcessor;
	};

	template<>
	struct TypeSerializer<Parent>
	{
		static void OnSerialize(Parent& parent, SerializationStream& stream)
		{
			stream.Serialize("ParentEntity", SerializationValue(parent.m_ParentEntity));
		}
	};

	struct ReparentEvent
	{
		FLARE_COMPONENT;

		Entity TargetEntity = Entity();
		Entity PreviousParent = Entity();
		Entity NewParent = Entity();
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
		static Entity DuplicateEntityHierarchy(World& world, Entity root, const std::unordered_set<ComponentId>* ignoredComponents);
	private:
		static Entity DuplicateEntityHierarchyRecursively(World& world,
			Entity root,
			const std::unordered_set<ComponentId>* ignoredComponents);

		static void ReattachCopyToOriginalParent(World& world, Entity originalEntity, Entity copy);

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
		Query m_NonLeafEntitiesQuery;
		Query m_LeafEntitiesQuery;
	};
}
