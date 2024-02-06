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
			: ParentEntity(Entity()), IndexInParent(0) {}

		Entity ParentEntity;
		uint32_t IndexInParent;
	};

	template<>
	struct TypeSerializer<Parent>
	{
		static void OnSerialize(Parent& parent, SerializationStream& stream)
		{
			stream.Serialize("ParentEntity", SerializationValue(parent.ParentEntity));
			stream.Serialize("IndexInParent", SerializationValue(parent.IndexInParent));
		}
	};

	

	class TransformPropagationSystem : public System
	{
	public:
		FLARE_SYSTEM;

		void OnConfig(World& world, SystemConfig& config) override;
		void OnUpdate(World& world, SystemExecutionContext& context) override;
	private:
		Query m_Qeury;
	};
}
