#pragma once

#include "FlareCore/Core.h"
#include "FlareCore/Serialization/TypeInitializer.h"

#include "FlareECS/Entity/Component.h"

#include <vector>

namespace Flare
{
	class FLAREECS_API Registry;
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

		friend class Registry;
	};
}

#define FLARE_COMPONENT                             \
	FLARE_TYPE                                      \
	static Flare::ComponentInitializer _Component;

#define FLARE_IMPL_COMPONENT(typeName, ...)                               \
	FLARE_IMPL_TYPE(typeName, __VA_ARGS__);                               \
	Flare::ComponentInitializer typeName::_Component(typeName::_Type);

#define COMPONENT_ID(typeName) (typeName::_Component.GetId())