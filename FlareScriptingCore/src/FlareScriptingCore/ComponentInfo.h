#pragma once

#include "FlareScriptingCore/Bindings/ECS/Component.h"

#include <string_view>
#include <vector>

namespace Flare::Internal
{
	struct ComponentInfo
	{
	public:
		ComponentInfo(std::string_view name)
			: Name(name), Id(INVALID_COMPONENT_ID)
		{
			GetRegisteredComponents().push_back(this);
		}
	public:
		static std::vector<ComponentInfo*>& GetRegisteredComponents();
	public:
		ComponentId Id;
		const std::string_view Name;
	};
}

#ifndef FLARE_SCRIPTING_CORE_NO_MACROS
	#define FLARE_COMPONENT(component) \
		FLARE_DEFINE_SCRIPTING_TYPE(component) \
		static Flare::ComponentInfo Info;

	#define FLARE_COMPONENT_IMPL(component) \
		FLARE_IMPL_SCRIPTING_TYPE(component) \
		Flare::ComponentInfo component::Info = Flare::ComponentInfo(typeid(component).name());
#endif
