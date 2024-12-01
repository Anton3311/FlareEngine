#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Serialization/TypeInitializer.h"
#include "FlareCore/Serialization/Serialization.h"
#include "FlareCore/Serialization/Metadata.h"

#include "FlareECS/Entity/Component.h"

#include <vector>

namespace Flare
{
	struct FLAREECS_API Components;
	class FLAREECS_API ComponentInitializer
	{
	public:
		ComponentInitializer(const TypeInitializer& type, const SerializableObjectDescriptor& serializationDescriptor);
		~ComponentInitializer();

		static std::vector<ComponentInitializer*>& GetInitializers();

		constexpr bool IsRegistered() const { return m_CorrespondingRegistry != nullptr; }
		constexpr ComponentId GetId() const { return m_Id; }
	public:
		const TypeInitializer& Type;
		const SerializableObjectDescriptor& SerializationDescriptor;
	private:
		ComponentId m_Id;
		Components* m_CorrespondingRegistry = nullptr;

		friend struct Components;
	};
}

#define FLARE_COMPONENT                             \
	FLARE_TYPE                                      \
	FLARE_SERIALIZABLE                              \
	static Flare::ComponentInitializer _Component;

#define FLARE_IMPL_COMPONENT(typeName)                                \
	FLARE_IMPL_TYPE(typeName);                                        \
	FLARE_SERIALIZABLE_IMPL(typeName);                                \
	Flare::ComponentInitializer typeName::_Component(typeName::_Type, FLARE_SERIALIZATION_DESCRIPTOR_OF(typeName));

#define COMPONENT_ID(typeName) (typeName::_Component.GetId())