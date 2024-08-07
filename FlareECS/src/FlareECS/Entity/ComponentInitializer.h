#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Serialization/TypeInitializer.h"

#include "FlareECS/Entity/Component.h"

#include <vector>

namespace Flare
{
	struct FLAREECS_API Components;
	class FLAREECS_API ComponentInitializer
	{
	public:
		ComponentInitializer(const TypeInitializer& type);
		~ComponentInitializer();

		static std::vector<ComponentInitializer*>& GetInitializers();

		constexpr ComponentId GetId() const { return m_Id; }
	public:
		const TypeInitializer& Type;
	private:
		ComponentId m_Id;

		friend struct Components;
	};
}

#define FLARE_COMPONENT                             \
	FLARE_TYPE                                      \
	static Flare::ComponentInitializer _Component;

#define FLARE_IMPL_COMPONENT(typeName)                                \
	FLARE_IMPL_TYPE(typeName);                                        \
	Flare::ComponentInitializer typeName::_Component(typeName::_Type);

#define COMPONENT_ID(typeName) (typeName::_Component.GetId())