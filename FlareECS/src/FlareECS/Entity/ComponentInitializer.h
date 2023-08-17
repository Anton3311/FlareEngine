#pragma once

#include "Flare/Core/Core.h"
#include "Flare/Serialization/TypeInitializer.h"

#include "FlareECS/ComponentId.h"

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

#define FLARE_COMPONENT2	                            \
	FLARE_TYPE                                      \
	static Flare::ComponentInitializer _Component;

#define FLARE_IMPL_COMPONENT2(typeName, ...)                               \
	FLARE_IMPL_TYPE(typeName, __VA_ARGS__);                               \
	Flare::ComponentInitializer typeName::_Component(typeName::_Type);
